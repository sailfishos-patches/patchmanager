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

#include "patchmanager.h"
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtQml/qqml.h>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>
#include <QLocale>
#include <QtDBus/QtDBus>
#include "webcatalog.h"

static const char *HOMESCREEN_CODE = "homescreen";
static const char *MESSAGES_CODE = "messages";
static const char *PHONE_CODE = "phone";
static const char *SILICA_CODE = "silica";

static void toggleSet(QSet<QString> &set, const QString &entry)
{
    if (set.contains(entry)) {
        set.remove(entry);
    } else {
        set.insert(entry);
    }
}

PatchManager::PatchManager(QObject *parent)
    : QObject(parent), m_appsNeedRestart(false), m_homescreenNeedRestart(false)
{
    m_nam = new QNetworkAccessManager(this);
    m_settings = new QSettings("/home/nemo/.config/patchmanager2.conf", QSettings::IniFormat, this);
}

PatchManager *PatchManager::GetInstance(QObject *parent)
{
    static PatchManager* lsSingleton = NULL;
    if (!lsSingleton) {
        lsSingleton = new PatchManager(parent);
    }
    return lsSingleton;
}

bool PatchManager::isAppsNeedRestart() const
{
    return m_appsNeedRestart;
}

bool PatchManager::isHomescreenNeedRestart() const
{
    return m_homescreenNeedRestart;
}

QString PatchManager::serverMediaUrl()
{
    return QString(MEDIA_URL);
}

bool PatchManager::developerMode()
{
    return getSettings("developerMode", false).toBool();
}

void PatchManager::setDeveloperMode(bool developerMode)
{
    if (putSettings("developerMode", developerMode)) {
        emit developerModeChanged(developerMode);
    }
}

void PatchManager::onDownloadFinished(const QString &patch, const QString &fileName)
{
    WebDownloader * download = qobject_cast<WebDownloader*>(sender());
    if (download) {
        download->deleteLater();
    }
    emit downloadFinished(patch, fileName);
}

void PatchManager::onServerReplied()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        reply->deleteLater();
        emit serverReply();
    }
}

void PatchManager::onEasterReply()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            if (reply->bytesAvailable()) {
                QByteArray json = reply->readAll();

                QJsonParseError error;
                QJsonDocument document = QJsonDocument::fromJson(json, &error);

                if (error.error == QJsonParseError::NoError) {
                    const QJsonObject & object = document.object();
                    if (object.value("status").toBool()) {
                        emit easterReceived(object.value("text").toString());
                    }
                }
            }
        }
        reply->deleteLater();
    }
}

void PatchManager::patchToggleService(const QString &patch, const QString &code)
{
    if (code == HOMESCREEN_CODE || code == SILICA_CODE) {
        toggleSet(m_homescreenPatches, patch);
    } else if (code == MESSAGES_CODE) {
        toggleSet(m_messagesPatches, patch);
    } else if (code == PHONE_CODE) {
        toggleSet(m_voiceCallPatches, patch);
    }

    bool newAppsNeedRestart = (!m_messagesPatches.isEmpty() || !m_voiceCallPatches.isEmpty());
    bool newHomescreenNeedRestart = !m_homescreenPatches.isEmpty();

    if (m_appsNeedRestart != newAppsNeedRestart) {
        m_appsNeedRestart = newAppsNeedRestart;
        emit appsNeedRestartChanged();
    }

    if (m_homescreenNeedRestart != newHomescreenNeedRestart) {
        m_homescreenNeedRestart = newHomescreenNeedRestart;
        emit homescreenNeedRestartChanged();
    }
}

void PatchManager::restartServices()
{
    if (!m_messagesPatches.isEmpty()) {
        QStringList arguments;
        arguments << "jolla-messages";
        QProcess::execute("killall", arguments);
        m_messagesPatches.clear();
    }

    if (!m_voiceCallPatches.isEmpty()) {
        QStringList arguments;
        arguments << "voicecall-ui";
        QProcess::execute("killall", arguments);
        m_voiceCallPatches.clear();
    }

    if (m_appsNeedRestart) {
        m_appsNeedRestart = false;
        emit appsNeedRestartChanged();
    }

    if (m_homescreenNeedRestart) {
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                        QStringLiteral("/org/freedesktop/systemd1/unit/lipstick_2eservice"),
                                                        QStringLiteral("org.freedesktop.systemd1.Unit"),
                                                        QStringLiteral("Restart"));
        m.setArguments({ QStringLiteral("replace") });
        QDBusConnection::sessionBus().send(m);
        m_homescreenNeedRestart = false;
        emit homescreenNeedRestartChanged();
    }
}

void PatchManager::downloadPatch(const QString &patch, const QString &destination, const QString &patchUrl)
{
    WebDownloader * download = new WebDownloader(this);
    download->patch = patch;
    download->url = patchUrl;
    download->destination = destination;
    QObject::connect(download, SIGNAL(downloadFinished(QString,QString)), this, SLOT(onDownloadFinished(QString,QString)));
    download->start();
}

bool PatchManager::installTranslator(const QString &patch)
{
    if (!m_translators.contains(patch)) {
        QTranslator * translator = new QTranslator(this);
        translator->load(QLocale::system(), QString("translation"), QString("_"), QString("/usr/share/patchmanager/patches/%1").arg(patch), QString(".qm"));
        bool ok = qGuiApp->installTranslator(translator);
        if (ok) {
            m_translators[patch] = translator;
        }
        return ok;
    }
    return true;
}

bool PatchManager::removeTranslator(const QString &patch)
{
    if (m_translators.contains(patch)) {
        QTranslator * translator = m_translators.take(patch);
        translator->deleteLater();
        return qGuiApp->removeTranslator(translator);
    }
    return true;
}

void PatchManager::activation(const QString &patch, const QString &version)
{
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", patch);
    query.addQueryItem("version", version);
    query.addQueryItem("action", "activation");
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &PatchManager::onServerReplied);
}

int PatchManager::checkVote(const QString &patch)
{
    QString key = QString("votes/%1").arg(patch);
    return m_settings->value(key, 0).toInt();
}

void PatchManager::doVote(const QString &patch, int action)
{
    if (checkVote(patch) == action) {
        return;
    }

    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", patch);
    if (action == 0) {
        query.addQueryItem("action", checkVote(patch) == 1 ? "upvote" : "downvote");
    } else {
        query.addQueryItem("action", action == 1 ? "downvote" : "upvote");
        if (checkVote(patch) > 0) {
            query.addQueryItem("twice", "true");
        }
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &PatchManager::onServerReplied);

    QString key = QString("votes/%1").arg(patch);
    m_settings->setValue(key, action);
    m_settings->sync();
}

void PatchManager::checkEaster()
{
    QUrl url(CATALOG_URL"/easter");
    QNetworkRequest request(url);
    QNetworkReply * reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &PatchManager::onEasterReply);
}

QString PatchManager::valueIfExists(const QString &filename)
{
    if (QFile(filename).exists()) {
        return filename;
    }
    return QString();
}

bool PatchManager::callUninstallOldPatch(const QString &patch)
{
    QString patchPath = QString("/usr/share/patchmanager/patches/%1/unified_diff.patch").arg(patch);
    if (QFile(patchPath).exists()) {
        QProcess proc;
        proc.start("/bin/rpm", QStringList() << "-qf" << "--qf" << "%{NAME}" << patchPath);
        if (proc.waitForFinished(5000) && proc.exitCode() == 0) {
            QString package = QString::fromLatin1(proc.readAllStandardOutput());
            if (!package.isEmpty()) {
                QDBusInterface iface("com.jolla.jollastore", "/StoreClient", "com.jolla.jollastore", QDBusConnection::sessionBus());
                iface.call(QDBus::NoBlock, "removePackage", package, true);
                return true;
            }
        } else {
            proc.kill();
        }
    }
    return false;
}

bool PatchManager::putSettings(const QString &name, const QVariant &value)
{
    QString key = QString("settings/%1").arg(name);
    QVariant old = m_settings->value(key);
    if (old != value) {
        m_settings->setValue(key ,value);
        return true;
    }
    return false;
}

QVariant PatchManager::getSettings(const QString &name, const QVariant &def)
{
    QString key = QString("settings/%1").arg(name);
    return m_settings->value(key ,def);
}
