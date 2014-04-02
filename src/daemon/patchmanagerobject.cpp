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
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QProcess>
#include <QtDBus/QDBusConnection>

static const char *SERVICE = "org.SfietKonstantin.patchmanager";
static const char *PATH = "/org/SfietKonstantin/patchmanager";
static const char *PATCHES_DIR = "/usr/share/patchmanager/patches";
static const char *CONFIG_FILE = "/var/lib/patchmanager/applied_patches.json";
static const char *PATCH_FILE = "patch.json";
static const char *PATCH_KEY = "patch";
static const char *DIR_KEY = "dir";

static const char *PATCHES_KEY = "patches";
static const char *PATCH = "patch";
static const char *P1_FLAG = "-p1";
static const char *T_FLAG = "-t";
static const char *N_FLAG = "-N";
static const char *F_FLAG = "-f";
static const char *R_FLAG = "-R";
static const char *DRY_RUN_FLAG = "--dry-run";

static inline bool performPatchApplication(const QString &patch, const QString &workingDirectory,
                                           bool reverse = false)
{
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
    if (process.exitCode() != 0) {
        return false;
    }

    arguments.clear();
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

static bool saveAppliedPatches(const QSet<QString> &patches)
{
    QJsonDocument document;
    QJsonArray array;
    foreach (const QString &patch, patches) {
        array.append(QJsonValue(patch));
    }
    document.setArray(array);

    QFile file (CONFIG_FILE);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(document.toJson());
    file.close();
    return true;
}

PatchManagerObject::PatchManagerObject(QObject *parent) :
    QObject(parent), m_dbusRegistered(false)
{
    // Applied patchs
    QFile file (CONFIG_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
        QCoreApplication::quit();
        return;
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        QCoreApplication::quit();
        return;
    }

    QJsonArray array = document.array();
    foreach (const QJsonValue &value, array) {
        m_appliedPatches.insert(value.toString());
    }
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

        if (!connection.registerObject(PATH, this, QDBusConnection::ExportAllSlots)) {
            QCoreApplication::quit();
            return;
        }
        m_dbusRegistered = true;
    }
}

QStringList PatchManagerObject::listPatches() const
{
    QDir root (PATCHES_DIR);
    return root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
}

bool PatchManagerObject::isPatchApplied(const QString &patch)
{
    return m_appliedPatches.contains(patch);
}

bool PatchManagerObject::applyPatch(const QString &patch)
{
    QDir root (PATCHES_DIR);
    QStringList patches = root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    if (!patches.contains(patch)) {
        return false;
    }

    if (!root.cd(patch)) {
        return false;
    }

    QFile file (root.absoluteFilePath(PATCH_FILE));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QList<QJsonObject> successedPatches;
    bool ok = true;
    foreach (const QJsonValue &value, document.object().value(PATCHES_KEY).toArray()) {
        QJsonObject object = value.toObject();
        QString patchFile = object.value(PATCH_KEY).toString();
        QString workingDirectory = object.value(DIR_KEY).toString();
        if (performPatchApplication(root.absoluteFilePath(patchFile), workingDirectory)) {
            successedPatches.append(object);
        } else {
            ok = false;
            break;
        }
    }

    if (!ok) {
        foreach (const QJsonObject &object, successedPatches) {
            QString patchFile = object.value(PATCH_KEY).toString();
            QString workingDirectory = object.value(DIR_KEY).toString();
            performPatchApplication(root.absoluteFilePath(patchFile), workingDirectory, true);
        }
        return false;
    }

    m_appliedPatches.insert(patch);
    saveAppliedPatches(m_appliedPatches);
    return true;
}

bool PatchManagerObject::unapplyPatch(const QString &patch)
{
    QDir root (PATCHES_DIR);
    QStringList patches = root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    if (!patches.contains(patch)) {
        return false;
    }

    if (!root.cd(patch)) {
        return false;
    }

    QFile file (root.absoluteFilePath(PATCH_FILE));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QList<QJsonObject> successedPatches;
    bool ok = true;
    foreach (const QJsonValue &value, document.object().value(PATCHES_KEY).toArray()) {
        QJsonObject object = value.toObject();
        QString patchFile = object.value(PATCH_KEY).toString();
        QString workingDirectory = object.value(DIR_KEY).toString();
        if (performPatchApplication(root.absoluteFilePath(patchFile), workingDirectory, true)) {
            successedPatches.append(object);
        } else {
            ok = false;
            break;
        }
    }

    if (!ok) {
        foreach (const QJsonObject &object, successedPatches) {
            QString patchFile = object.value(PATCH_KEY).toString();
            QString workingDirectory = object.value(DIR_KEY).toString();
            performPatchApplication(root.absoluteFilePath(patchFile), workingDirectory, false);
        }
        return false;
    }

    m_appliedPatches.remove(patch);
    saveAppliedPatches(m_appliedPatches);
    return true;
}

void PatchManagerObject::unapplyAllPatches()
{
    QStringList appliedPatches = m_appliedPatches.toList();
    foreach (const QString &patch, appliedPatches) {
        unapplyPatch(patch);
    }
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
