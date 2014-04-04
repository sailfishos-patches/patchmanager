/*
 * Copyright (C) 2014 Lucien XU <sfietkonstantin@free.fr>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "patchmanagerobject.h"
#include <algorithm>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QProcess>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusVariant>
#include <notification.h>

// How patches are applied
//
// When applying a patch, first, we will try to
// dry-run all the patches. If this step is successful
// we make a copy the patch to /var/lib/patchmanager/patches/<folder>
// We then display a notification, that the patch will
// be applied, before applying the patch.
//
// To unapply a patch, we first try to dry-run the
// patches. If this step is successful, we unapply the
// patch, displaying a notification as well. However, if
// the patch fails to unapply, we keep it applied, and
// display an error

// If patches that should be unapplied are not available,
// we use the /var/lib/patchmanager/patches/ to unapply
// patches.


static const char *SERVICE = "org.SfietKonstantin.patchmanager";
static const char *PATH = "/org/SfietKonstantin/patchmanager";
static const char *PATCHES_DIR = "/usr/share/patchmanager/patches";
static const char *APPLIED_PATCHES_DIR = "/var/lib/patchmanager/patches";
static const char *PATCH_FILE = "patch.json";
static const char *PATCH_KEY = "patch";
static const char *DIR_KEY = "dir";

static const char *NAME_KEY = "name";
static const char *DESCRIPTION_KEY = "description";
static const char *CATEGORY_KEY = "category";
static const char *INFOS_KEY = "infos";
static const char *PATCHES_KEY = "patches";
static const char *PATCH = "patch";
static const char *P1_FLAG = "-p1";
static const char *T_FLAG = "-t";
static const char *N_FLAG = "-N";
static const char *F_FLAG = "-f";
static const char *R_FLAG = "-R";
static const char *DRY_RUN_FLAG = "--dry-run";

bool patchSort(const Patch &patch1, const Patch &patch2)
{
    if (patch1.category == patch2.category) {
        return patch1.name < patch2.name;
    }

    return patch1.category < patch2.category;
}

QDBusArgument &operator<<(QDBusArgument &argument, const Patch &patch)
{
    argument.beginStructure();
    argument << patch.patch << patch.name << patch.description << patch.category << patch.available;
    argument.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QDBusVariant>());
    foreach (const QString &key, patch.infos.keys()) {
        argument.beginMapEntry();
        argument << key << QDBusVariant(patch.infos.value(key));
        argument.endMapEntry();
    }
    argument.endMap();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Patch &patch)
{
    argument.beginStructure();
    argument >> patch.patch >> patch.name >> patch.description >> patch.category >> patch.available;
    patch.infos.clear();
    argument.beginMap();
    while (!argument.atEnd()) {
        argument.beginMapEntry();
        QString key;
        QDBusVariant value;
        argument >> key >> value;
        patch.infos.insert(key, value.variant());
        argument.endMapEntry();
    }
    argument.endMap();
    argument.endStructure();
    return argument;
}

static inline bool makePatch(const QDir &root, const QString &patchPath, Patch *patch, bool available)
{
    QDir patchDir (root);
    if (!patchDir.cd(patchPath)) {
        return false;
    }

    QFile file (patchDir.absoluteFilePath(PATCH_FILE));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    const QJsonObject &object = document.object();
    QString name = object.value(NAME_KEY).toString().trimmed();
    QString description = object.value(DESCRIPTION_KEY).toString().trimmed();
    QString category = object.value(CATEGORY_KEY).toString().trimmed();

    if (name.isEmpty() || description.isEmpty() || category.isEmpty()) {
        return false;
    }

    patch->patch = patchPath;
    patch->name = name;
    patch->description = description;
    patch->category = category;
    patch->available = available;
    patch->infos.clear();
    QJsonObject infos = object.value(INFOS_KEY).toObject();
    foreach (const QString &key, infos.keys()) {
        patch->infos.insert(key, infos.value(key).toVariant());
    }

    foreach (const QJsonValue &value, object.value(PATCHES_KEY).toArray()) {
        QJsonObject object = value.toObject();
        PatchInfo patchInfo;

        patchInfo.patchFile = object.value(PATCH_KEY).toString();
        // Check if the patches are in the correct directory
        if (!patchDir.exists(patchInfo.patchFile)) {
            return false;
        }
        patchInfo.workingDirectory = object.value(DIR_KEY).toString();

        patch->patchInfos.append(patchInfo);
    }

    return true;
}

static inline QList<Patch> listPatchesFromDir(const QString &dir, bool available,
                                              QMap<QString, Patch> &cache,
                                              const QSet<QString> &existing)
{
    QList<Patch> patches;

    QDir root (dir);
    foreach (const QString &patchPath, root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        Patch patch;
        bool ok = makePatch(root, patchPath, &patch, available);
        if (ok) {
            cache.insert(patch.patch, patch);
            if (!existing.contains(patch.patch)) {
                patches.append(patch);
            }
        }
    }
    return patches;
}

// Return true if the patch is in the given config
// else, return false
static inline bool performDryRun(const QString &patch, const QString &workingDirectory,
                                bool reverse = false)
{
    if (!QFile::exists(patch)) {
        return false;
    }

    // Dry run
    QProcess process;
    process.setProgram(PATCH);
    process.setWorkingDirectory(workingDirectory);
    process.setStandardInputFile(patch);
    QStringList arguments;
    arguments << P1_FLAG << DRY_RUN_FLAG;
    if (!reverse) {
        arguments << T_FLAG << N_FLAG;
    } else {
        arguments << F_FLAG << R_FLAG;
    }
    process.setArguments(arguments);
    process.start();
    process.waitForFinished(-1);
    return (process.exitCode() == 0);
}

// Return true if the patch operation is done
// else return false
static inline bool performPatchApplication(const QString &patch, const QString &workingDirectory,
                                           bool reverse = false)
{
    if (!QFile::exists(patch)) {
        return false;
    }

    QProcess process;
    process.setProgram(PATCH);
    process.setWorkingDirectory(workingDirectory);
    process.setStandardInputFile(patch);
    QStringList arguments;
    arguments << P1_FLAG;
    if (!reverse) {
        arguments << T_FLAG << N_FLAG;
    } else {
        arguments << F_FLAG << R_FLAG;
    }
    process.setArguments(arguments);
    process.start();
    process.waitForFinished(-1);
    return (process.exitCode() == 0);
}

static bool addAppliedPatch(const Patch &patch)
{
    QDir root (APPLIED_PATCHES_DIR);
    if (root.exists(patch.patch)) {
        return false;
    }

    if (!root.mkdir(patch.patch)) {
        return false;
    }

    if (!root.cd(patch.patch)) {
        return false;
    }

    // Copy patch file
    QDir original (PATCHES_DIR);
    if (!original.cd(patch.patch)) {
        return false;
    }

    if (!QFile::copy(original.absoluteFilePath(PATCH_FILE), root.absoluteFilePath(PATCH_FILE))) {
        return false;
    }

    // Copy patches
    qDebug() << patch.patchInfos.count() << patch.patchInfos.first().patchFile;
    foreach (const PatchInfo &patchInfo, patch.patchInfos) {
        if (!QFile::copy(original.absoluteFilePath(patchInfo.patchFile),
                         root.absoluteFilePath(patchInfo.patchFile))) {
            return false;
        }
    }

    return true;
}

static bool rmAppliedPatch(const Patch &patch)
{
    QDir root (APPLIED_PATCHES_DIR);
    if (!root.exists(patch.patch)) {
        return false;
    }

    if (!root.cd(patch.patch)) {
        return false;
    }

    // Rm patch file
    if (!root.remove(PATCH_FILE)) {
        return false;
    }

    // Rm patches
    foreach (const PatchInfo &patchInfo, patch.patchInfos) {
        if (!root.remove(patchInfo.patchFile)) {
            return false;
        }
    }

    root = QDir(APPLIED_PATCHES_DIR);
    if (!root.rmdir(patch.patch)) {
        return false;
    }

    return true;
}

static inline QString makePath(const QString &root, const QString &patch, const QString &file)
{
    QDir dir (root);
    if (!dir.cd(patch)) {
        return QString();
    }

    if (!dir.exists(file)) {
        return QString();
    }

    return dir.absoluteFilePath(file);
}

static inline void notify(const QString &body)
{
    Notification *notification = new Notification();
    notification->setHintValue("x-nemo-icon", "icon-l-developer-mode");
    notification->setHintValue("x-nemo-preview-icon", "icon-l-developer-mode");
    notification->setSummary("patchmanager");
    notification->setBody(body);
    notification->setPreviewSummary("patchmanager");
    notification->setPreviewBody(body);
    notification->setTimestamp(QDateTime::currentDateTime());
    notification->publish();
}

PatchManagerObject::PatchManagerObject(QObject *parent) :
    QObject(parent), m_dbusRegistered(false)
{
    QDir root (APPLIED_PATCHES_DIR);
    if (!root.exists()) {
        QDir::root().mkpath(APPLIED_PATCHES_DIR);
    }

    QStringList patches = root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    foreach (const QString &patch, patches) {
        m_appliedPatches.insert(patch);
    }

    refreshPatchList();
}

PatchManagerObject::~PatchManagerObject()
{
    if (m_dbusRegistered) {
        QDBusConnection connection = QDBusConnection::systemBus();
        connection.unregisterObject(PATH);
        connection.unregisterService(SERVICE);
    }
}

void PatchManagerObject::registerDBus()
{
    if (!m_dbusRegistered) {
        qDBusRegisterMetaType<Patch>();
        qDBusRegisterMetaType<QList<Patch> >();

        // DBus
        QDBusConnection connection = QDBusConnection::systemBus();
        if (!connection.registerService(SERVICE)) {
            QCoreApplication::quit();
            return;
        }

        if (!connection.registerObject(PATH, this)) {
            QCoreApplication::quit();
            return;
        }
        m_dbusRegistered = true;
    }
}

QList<Patch> PatchManagerObject::listPatches()
{
    return m_patches;
}

bool PatchManagerObject::isPatchApplied(const QString &patch)
{
    return m_appliedPatches.contains(patch);
}

bool PatchManagerObject::canApplyPatch(const QString &patch)
{
    if (!m_cachedPatches.contains(patch)) {
        return false;
    }

    const Patch &patchData = m_cachedPatches[patch];
    foreach (const PatchInfo &patchInfo, patchData.patchInfos) {
        if (!performDryRun(makePath(PATCHES_DIR, patchData.patch, patchInfo.patchFile),
                           patchInfo.workingDirectory)) {
            return false;
        }
    }

    return true;
}

bool PatchManagerObject::canUnapplyPatch(const QString &patch)
{
    Patch patchData;
    QString dir;
    if (!m_cachedAppliedPatches.contains(patch)) {
        if (!m_cachedPatches.contains(patch)) {
            return false;
        }
        dir = PATCHES_DIR;
        patchData = m_cachedPatches.value(patch);
    } else {
        dir = APPLIED_PATCHES_DIR;
        patchData = m_cachedAppliedPatches.value(patch);
    }

    foreach (const PatchInfo &patchInfo, m_cachedAppliedPatches.value(patch).patchInfos) {
        if (!performDryRun(makePath(dir, patchData.patch, patchInfo.patchFile),
                           patchInfo.workingDirectory, true)) {
            return false;
        }
    }

    return true;
}

bool PatchManagerObject::applyPatch(const QString &patch)
{
    if (!canApplyPatch(patch)) {
        return false;
    }

    Patch patchData = m_cachedPatches.value(patch);
    foreach (const PatchInfo &patchInfo, patchData.patchInfos) {
        if (!performPatchApplication(makePath(PATCHES_DIR, patchData.patch, patchInfo.patchFile),
                                     patchInfo.workingDirectory)) {
            return false;
        }
    }

    m_appliedPatches.insert(patchData.patch);
    m_cachedAppliedPatches.insert(patchData.patch, patchData);
    addAppliedPatch(patchData);
    notify(QString("Applied patch: %1").arg(patchData.name));
    return true;
}

bool PatchManagerObject::unapplyPatch(const QString &patch)
{
    if (!canUnapplyPatch(patch)) {
        return false;
    }

    Patch patchData;
    QString dir;
    if (!m_cachedAppliedPatches.contains(patch)) {
        if (!m_cachedPatches.contains(patch)) {
            return false;
        }
        dir = PATCHES_DIR;
        patchData = m_cachedPatches.value(patch);
    } else {
        dir = APPLIED_PATCHES_DIR;
        patchData = m_cachedAppliedPatches.value(patch);
    }

    foreach (const PatchInfo &patchInfo, patchData.patchInfos) {
        if (!performPatchApplication(makePath(dir, patchData.patch, patchInfo.patchFile),
                                     patchInfo.workingDirectory, true)) {
            return false;
        }
    }

    m_appliedPatches.remove(patchData.patch);
    m_cachedAppliedPatches.remove(patchData.patch);
    if (patchData.available) {
        m_cachedPatches.insert(patchData.patch, patchData);
    }
    rmAppliedPatch(patchData);
    notify(QString("Removed patch: %1").arg(patchData.name));
    return true;
}

void PatchManagerObject::unapplyAllPatches()
{
    checkPatches();

    QStringList appliedPatches = m_appliedPatches.toList();
    foreach (const QString &patch, appliedPatches) {
        unapplyPatch(patch);
    }
    quit();
}

void PatchManagerObject::checkPatches()
{
    QList<Patch> patches = listPatches();
    foreach (const Patch &patch, patches) {
        bool canApply = canApplyPatch(patch.patch);
        bool canUnapply = canUnapplyPatch(patch.patch);
        bool isApplied = isPatchApplied(patch.patch);

        // A U I -> problem
        // A U i -> problem
        // A u I -> rm
        // A u i -> ok
        // a U I -> add
        // a U i -> ok
        // a u I -> problem
        // a u i -> problem

        if (canApply && !canUnapply) {
            if (isApplied) {
                // Remove the patch
                rmAppliedPatch(patch);
                m_appliedPatches.remove(patch.patch);
            }
        } else if (!canApply && canUnapply) {
            if (!isApplied) {
                // Add the patch
                addAppliedPatch(patch);
                m_appliedPatches.insert(patch.patch);
            }
        } else {
            qDebug() << "Issue with patch" << patch.patch << "Can apply" << canApply
                     << "Can unapply" << canUnapply;
        }
    }

    refreshPatchList();
}

void PatchManagerObject::installLipstickPandora()
{
    QProcess::execute("mv /home/nemo/lipstick-pandora/:qml /opt/lipstick-pandora/qml");
    QProcess::execute("rm -r /home/nemo/lipstick-pandora");
}

void PatchManagerObject::uninstallLipstickPandora()
{
    QProcess::execute("rm -r /opt/lipstick-pandora/qml");
}

void PatchManagerObject::quit()
{
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

bool PatchManagerObject::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        e->accept();
        QCoreApplication::quit();
        return true;
    }
    return QObject::event(e);
}

void PatchManagerObject::refreshPatchList()
{
    m_patches = listPatchesFromDir(PATCHES_DIR, true, m_cachedPatches, QSet<QString>());
    QSet<QString> existing;
    foreach (const Patch &patch, m_patches) {
        existing.insert(patch.patch);
    }

    m_patches.append(listPatchesFromDir(APPLIED_PATCHES_DIR, false, m_cachedAppliedPatches, existing));
    std::sort(m_patches.begin(), m_patches.end(), patchSort);
}
