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
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
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
static const char *PATCHES_ADDITIONAL_DIR = "/var/lib/patchmanager/ausmt/patches";
static const char *PATCH_FILE = "patch.json";

static const char *NAME_KEY = "name";
static const char *DESCRIPTION_KEY = "description";
static const char *CATEGORY_KEY = "category";
static const char *INFOS_KEY = "infos";

static const char *AUSMT_INSTALLED_LIST_FILE = "/var/lib/patchmanager/ausmt/packages";
static const char *AUSMT_INSTALL = "/opt/ausmt/ausmt-install";
static const char *AUSMT_REMOVE = "/opt/ausmt/ausmt-remove";

static const char *BROWSER_CODE = "browser";
static const char *CAMERA_CODE = "camera";
static const char *CALENDAR_CODE = "calendar";
static const char *CLOCK_CODE = "clock";
static const char *CONTACTS_CODE = "contacts";
static const char *EMAIL_CODE = "email";
static const char *GALLERY_CODE = "gallery";
static const char *HOMESCREEN_CODE = "homescreen";
static const char *MEDIA_CODE = "media";
static const char *MESSAGES_CODE = "messages";
static const char *PHONE_CODE = "phone";
static const char *SILICA_CODE = "silica";
static const char *SETTINGS_CODE = "settings";

static QString categoryFromCode(const QString &code)
{
    if (code == BROWSER_CODE) {
        return QObject::tr("Browser");
    } else if (code == CAMERA_CODE) {
        return QObject::tr("Camera");
    } else if (code == CALENDAR_CODE) {
        return QObject::tr("Calendar");
    } else if (code == CLOCK_CODE) {
        return QObject::tr("Clock");
    } else if (code == CONTACTS_CODE) {
        return QObject::tr("Contacts");
    } else if (code == EMAIL_CODE) {
        return QObject::tr("Email");
    } else if (code == GALLERY_CODE) {
        return QObject::tr("Gallery");
    } else if (code == HOMESCREEN_CODE) {
        return QObject::tr("Homescreen");
    } else if (code == MEDIA_CODE) {
        return QObject::tr("Media");
    } else if (code == MESSAGES_CODE) {
        return QObject::tr("Messages");
    } else if (code == PHONE_CODE) {
        return QObject::tr("Phone");
    } else if (code == SILICA_CODE) {
        return QObject::tr("Silica");
    } else if (code == SETTINGS_CODE) {
        return QObject::tr("Settings");
    }
    return QObject::tr("Other");
}

static bool patchSort(const QVariant &patch1, const QVariant &patch2)
{
    if (patch1.toMap()["category"].toString() == patch2.toMap()["category"].toString()) {
        return patch1.toMap()["name"].toString() < patch2.toMap()["name"].toString();
    }

    return patch1.toMap()["category"].toString() < patch2.toMap()["category"].toString();
}

bool PatchManagerObject::makePatch(const QDir &root, const QString &patchPath, QVariantMap &patch, bool available)
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

    QVariantMap json = document.toVariant().toMap();

    const QStringList &keys = json.keys();

    if (!keys.contains(NAME_KEY) || !keys.contains(DESCRIPTION_KEY) || !keys.contains(CATEGORY_KEY)) {
        return false;
    }

    json["patch"] = patchPath;
    json["available"] = available;
    json["categoryCode"] = json["category"];
    json["category"] = categoryFromCode(json["category"].toString());
    json["patched"] = m_appliedPatches.contains(patchPath);
    if (!json.contains("version")) {
        json["version"] = "0.0.0";
    }
    patch = json;

    return true;
}

QVariantList PatchManagerObject::listPatchesFromDir(const QString &dir, QSet<QString> &existingPatches, bool existing)
{
    QVariantList patches;
    QDir root (dir);
    foreach (const QString &patchPath, root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        if (!existingPatches.contains(patchPath)) {
            QVariantMap patch;
            bool ok = makePatch(root, patchPath, patch, existing);
            if (ok) {
                patches.append(patch);
                existingPatches.insert(patchPath);
            }
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

QVariantList PatchManagerObject::listPatches()
{
    m_timer->start();
    return m_patches;
}

QVariantMap PatchManagerObject::listVersions()
{
    m_timer->start();
    return m_versions;
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
        refreshPatchList();
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
        refreshPatchList();
    }

    m_timer->start();
    return ok;
}

bool PatchManagerObject::unapplyAllPatches()
{
    m_timer->stop();
    bool ok = true;
    QStringList appliedPatches = m_appliedPatches.toList();
    foreach (const QString &patch, appliedPatches) {
        ok &= unapplyPatch(patch);
    }
    return ok;
    m_timer->start();
}

bool PatchManagerObject::installPatch(const QString &patch, const QString &json, const QString &archive)
{
    m_timer->stop();
    QString patchPath = QString("%1/%2").arg(PATCHES_DIR).arg(patch);
    QString jsonPath = QString("%1/%2").arg(patchPath).arg(PATCH_FILE);
    QFile archiveFile(archive);
    QDir patchDir(patchPath);
    bool result = false;
    if (archiveFile.exists() && !patchDir.exists() && patchDir.mkpath(patchPath)) {
        QFile jsonFile(jsonPath);
        if (jsonFile.open(QFile::WriteOnly)) {
            jsonFile.write(json.toLatin1());
            jsonFile.close();

            QProcess proc;
            int ret = 0;
            if (archive.endsWith(".zip")) {
                ret = proc.execute("/usr/bin/unzip", QStringList() << archive << "-d" << patchPath);
            } else {
                ret = proc.execute("/bin/tar", QStringList() << "xzf" << archive << "-C" << patchPath);
            }
            if (ret == 0) {
                refreshPatchList();
                result = true;
            } else {
                patchDir.removeRecursively();
            }
        }
    }
    if (archiveFile.exists()) {
        archiveFile.remove();
    }
    m_timer->start();
    return result;
}

bool PatchManagerObject::uninstallPatch(const QString &patch)
{
    m_timer->stop();
    if (m_appliedPatches.contains(patch)) {
        unapplyPatch(patch);
        m_timer->stop();
    }

    QDir patchDir(QString("%1/%2").arg(PATCHES_DIR).arg(patch));
    if (patchDir.exists()) {
        bool ok = patchDir.removeRecursively();
        refreshPatchList();
        m_timer->start();
        return ok;
    }

    m_timer->start();
    return true;
}

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
    QSet<QString> existingPatches;
    m_patches = listPatchesFromDir(PATCHES_DIR, existingPatches);
    m_patches.append(listPatchesFromDir(PATCHES_ADDITIONAL_DIR, existingPatches, false));
    std::sort(m_patches.begin(), m_patches.end(), patchSort);
    m_versions.clear();
    foreach (const QVariant & patch, m_patches) {
        QVariantMap d_patch = patch.toMap();
        if (d_patch.contains("version")) {
            m_versions[d_patch["name"].toString()] = d_patch["version"];
        } else {
            m_versions[d_patch["name"].toString()] = "0.0.0";
        }
    }
}
