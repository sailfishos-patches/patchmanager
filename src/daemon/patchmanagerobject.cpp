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
#include <QtCore/QTimer>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusVariant>

static const char *SERVICE = "org.SfietKonstantin.patchmanager";
static const char *PATH = "/org/SfietKonstantin/patchmanager";
static const char *PATCHES_DIR = "/usr/share/patchmanager/patches";
static const char *PATCH_FILE = "patch.json";

static const char *NAME_KEY = "name";
static const char *DESCRIPTION_KEY = "description";
static const char *CATEGORY_KEY = "category";
static const char *INFOS_KEY = "infos";

static const char *AUSMT_INSTALLED_LIST_FILE = "/var/lib/patchmanager/ausmt/packages";
static const char *AUSMT_INSTALL = "/opt/ausmt/ausmt-install";
static const char *AUSMT_REMOVE = "/opt/ausmt/ausmt-remove";

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

    return true;
}

static inline QList<Patch> listPatchesFromDir(const QString &dir)
{
    QList<Patch> patches;
    QDir root (dir);
    foreach (const QString &patchPath, root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        Patch patch;
        bool ok = makePatch(root, patchPath, &patch, true);
        if (ok) {
            patches.append(patch);
        }
    }
    return patches;
}

PatchManagerObject::PatchManagerObject(QObject *parent) :
    QObject(parent), m_dbusRegistered(false)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PatchManagerObject::quit);
    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::VeryCoarseTimer);
    m_timer->setInterval(15000);  // Quit after 15s timeout
    m_timer->start();

    QFile file (AUSMT_INSTALLED_LIST_FILE);
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = QString::fromLocal8Bit(file.readLine());
            qDebug() << line;
            QStringList splitted = line.split(" ");
            if (splitted.count() == 2) {
                m_appliedPatches.insert(splitted.first());
            }
        }
        file.close();
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
    m_timer->start();
    return m_patches;
}

bool PatchManagerObject::isPatchApplied(const QString &patch)
{
    m_timer->start();
    return m_appliedPatches.contains(patch);
}

bool PatchManagerObject::applyPatch(const QString &patch)
{
    m_timer->stop();
    QProcess process;
    process.setProgram(AUSMT_INSTALL);

    QStringList arguments;
    arguments.append(patch);

    process.setArguments(arguments);
    process.start();
    process.waitForFinished(-1);

    bool ok = (process.exitCode() == 0);
    if (ok) {
        m_appliedPatches.insert(patch);
    }

    m_timer->start();
    return ok;
}

bool PatchManagerObject::unapplyPatch(const QString &patch)
{
    m_timer->stop();
    QProcess process;
    process.setProgram(AUSMT_REMOVE);

    QStringList arguments;
    arguments.append(patch);

    process.setArguments(arguments);
    process.start();
    process.waitForFinished(-1);

    bool ok = (process.exitCode() == 0);
    if (ok) {
        m_appliedPatches.remove(patch);
    }

    m_timer->start();
    return ok;
}

//void PatchManagerObject::unapplyAllPatches()
//{
//    checkPatches();

//    QStringList appliedPatches = m_appliedPatches.toList();
//    foreach (const QString &patch, appliedPatches) {
//        unapplyPatch(patch);
//    }
//    quit();
//}

//void PatchManagerObject::checkPatches()
//{
//    QList<Patch> patches = listPatches();
//    foreach (const Patch &patch, patches) {
//        bool canApply = canApplyPatch(patch.patch);
//        bool canUnapply = canUnapplyPatch(patch.patch);
//        bool isApplied = isPatchApplied(patch.patch);

//        // A U I -> problem
//        // A U i -> problem
//        // A u I -> rm
//        // A u i -> ok
//        // a U I -> add
//        // a U i -> ok
//        // a u I -> problem
//        // a u i -> problem

//        if (canApply && !canUnapply) {
//            if (isApplied) {
//                // Remove the patch
//                rmAppliedPatch(patch);
//                m_appliedPatches.remove(patch.patch);
//            }
//        } else if (!canApply && canUnapply) {
//            if (!isApplied) {
//                // Add the patch
//                addAppliedPatch(patch);
//                m_appliedPatches.insert(patch.patch);
//            }
//        } else {
//            qDebug() << "Issue with patch" << patch.patch << "Can apply" << canApply
//                     << "Can unapply" << canUnapply;
//        }
//    }

//    refreshPatchList();
//}

void PatchManagerObject::installLipstickPandora()
{
    m_timer->stop();
    QProcess::execute("mv /home/nemo/lipstick-pandora/:qml /opt/lipstick-pandora/qml");
    QProcess::execute("/usr/share/patchmanager/tools/pandora-md5sum.sh");
    QProcess::execute("rm -r /home/nemo/lipstick-pandora");
    m_timer->start();
}

void PatchManagerObject::uninstallLipstickPandora()
{
    m_timer->stop();
    QProcess::execute("rm -r /opt/lipstick-pandora/qml");
    m_timer->start();
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
    m_patches = listPatchesFromDir(PATCHES_DIR);
    std::sort(m_patches.begin(), m_patches.end(), patchSort);
}
