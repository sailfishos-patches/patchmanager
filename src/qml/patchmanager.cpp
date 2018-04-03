/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
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
#include "patchmanager_interface.h"

static const char *HOMESCREEN_CODE = "homescreen";
static const char *MESSAGES_CODE = "messages";
static const char *PHONE_CODE = "phone";
static const char *SILICA_CODE = "silica";

static const char *noop_strings[] = {
    QT_TRANSLATE_NOOP("Sections", "browser"),
    QT_TRANSLATE_NOOP("Sections", "camera"),
    QT_TRANSLATE_NOOP("Sections", "calendar"),
    QT_TRANSLATE_NOOP("Sections", "clock"),
    QT_TRANSLATE_NOOP("Sections", "contacts"),
    QT_TRANSLATE_NOOP("Sections", "email"),
    QT_TRANSLATE_NOOP("Sections", "gallery"),
    QT_TRANSLATE_NOOP("Sections", "homescreen"),
    QT_TRANSLATE_NOOP("Sections", "media"),
    QT_TRANSLATE_NOOP("Sections", "messages"),
    QT_TRANSLATE_NOOP("Sections", "phone"),
    QT_TRANSLATE_NOOP("Sections", "silica"),
    QT_TRANSLATE_NOOP("Sections", "settings"),
    QT_TRANSLATE_NOOP("Sections", "other"),
};

static void toggleSet(QSet<QString> &set, const QString &entry)
{
    if (set.contains(entry)) {
        set.remove(entry);
    } else {
        set.insert(entry);
    }
}

PatchManager::PatchManager(QObject *parent)
    : QObject(parent)
    , m_appsNeedRestart(false)
    , m_homescreenNeedRestart(false)
    , m_installedModel(new PatchManagerModel(this))
    , m_interface(new PatchManagerInterface(DBUS_SERVICE_NAME, DBUS_PATH_NAME, QDBusConnection::systemBus(), this))
    , m_nam(new QNetworkAccessManager(this))
    , m_settings(new QSettings(QStringLiteral("/home/nemo/.config/patchmanager2.conf"), QSettings::IniFormat, this))
{

    requestListPatches(QString(), false);
    connect(m_interface, &PatchManagerInterface::patchAltered, this, &PatchManager::requestListPatches);
}

PatchManager *PatchManager::GetInstance(QObject *parent)
{
    static PatchManager* lsSingleton = nullptr;
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
    return QStringLiteral(MEDIA_URL);
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

PatchManagerModel *PatchManager::installedModel()
{
    return m_installedModel;
}

QString PatchManager::trCategory(const QString &category) const
{
    const QString section = qApp->translate("Sections", category.toLatin1().constData());
    if (section == category) {
        return qApp->translate("Sections", "other");
    }
    return section;
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

void PatchManager::requestListPatches(const QString &patch, bool installed)
{
    qDebug() << Q_FUNC_INFO << patch << installed;

//    if (!patch.isEmpty() && !installed) {
//        m_installedModel->removePatch(patch);
//    }

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(m_interface->listPatches(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, patch, installed](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<QVariantList> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            return;
        }
        const QVariantList data = unwind(reply.value()).toList();
        m_installedModel->populateData(data, patch, installed);
        watcher->deleteLater();
    });
}

QDBusPendingCallWatcher* PatchManager::applyPatch(const QString &patch)
{
    return new QDBusPendingCallWatcher(m_interface->applyPatch(patch), this);
}

QDBusPendingCallWatcher* PatchManager::unapplyPatch(const QString &patch)
{
    return new QDBusPendingCallWatcher(m_interface->unapplyPatch(patch), this);
}

QDBusPendingCallWatcher *PatchManager::installPatch(const QString &patch, const QString &version, const QString &url)
{
    return new QDBusPendingCallWatcher(m_interface->installPatch(patch, version, url), this);
}

QDBusPendingCallWatcher *PatchManager::uninstallPatch(const QString &patch)
{
    return new QDBusPendingCallWatcher(m_interface->uninstallPatch(patch), this);
}

QDBusPendingCallWatcher *PatchManager::resetState(const QString &patch)
{
    return new QDBusPendingCallWatcher(m_interface->resetState(patch), this);
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
        translator->load(QLocale::system(),
                         QStringLiteral("translation"),
                         QStringLiteral("_"),
                         QStringLiteral("/usr/share/patchmanager/patches/%1").arg(patch),
                         QStringLiteral(".qm"));
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
    QString key = QStringLiteral("votes/%1").arg(patch);
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

    QString key = QStringLiteral("votes/%1").arg(patch);
    m_settings->setValue(key, action);
    m_settings->sync();
}

void PatchManager::checkEaster()
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(m_interface->checkEaster(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<QString> reply = *watcher;
        if (!reply.isError()) {
            emit easterReceived(reply.value());
        }
        watcher->deleteLater();
    });
}

QString PatchManager::iconForPatch(const QString &patch)
{
    const QString iconPlaceholder = QStringLiteral("/usr/share/patchmanager/patches/%1/main.%2").arg(patch);
    const QStringList validExtensions = { QStringLiteral("png"), QStringLiteral("svg") };
    for (const QString &extension : validExtensions) {
        QString filename = iconPlaceholder.arg(extension);
        if (QFileInfo::exists(filename)) {
            return filename;
        }
    }
    return QString();
}

QString PatchManager::valueIfExists(const QString &filename)
{
    if (QFile(filename).exists()) {
        return filename;
    }
    return QString();
}

void PatchManager::successCall(QJSValue callback, const QVariant &value)
{
    if (callback.isUndefined() || !callback.isCallable()) {
        return;
    }

    QJSValueList callbackArguments;
    callbackArguments << callback.engine()->toScriptValue(value);
    callback.call(callbackArguments);
}

void PatchManager::errorCall(QJSValue errorCallback, const QString &message)
{
    if (errorCallback.isUndefined() || !errorCallback.isCallable()) {
        return;
    }

    QJSValueList callbackArguments;
    callbackArguments << QJSValue(message);
    errorCallback.call(callbackArguments);
}

bool PatchManager::putSettings(const QString &name, const QVariant &value)
{
    QString key = QStringLiteral("settings/%1").arg(name);
    QVariant old = m_settings->value(key);
    if (old != value) {
        m_settings->setValue(key ,value);
        return true;
    }
    return false;
}

QVariant PatchManager::getSettings(const QString &name, const QVariant &def)
{
    QString key = QStringLiteral("settings/%1").arg(name);
    return m_settings->value(key ,def);
}

QVariant PatchManager::unwind(const QVariant &val, int depth)
{
    /* Limit recursion depth to protect against type conversions
     * that fail to converge to basic qt types within qt variant.
     *
     * Using limit >= DBUS_MAXIMUM_TYPE_RECURSION_DEPTH (=32) should
     * mean we do not bail out too soon on deeply nested but othewise
     * valid dbus messages. */
    static const int maximum_dept = 32;

    /* Default to QVariant with isInvalid() == true */
    QVariant res;

    const int type = val.userType();

    if( ++depth > maximum_dept ) {
        /* Leave result to invalid variant */
        qWarning() << "Too deep recursion detected at userType:" << type;
    }
    else if (type == QVariant::List) {
        /* Is built-in type, but does not get correctly converted
         * to qml domain if contains QDBus types inside -> convert
         * to variant list and unwind each item separately */
        QVariantList list;
        for (const QVariant &var: val.toList()) {
            list.append(unwind(var, depth));
        }
        res = list;
    }
    else if (type == QVariant::ByteArray ) {
        /* Is built-in type, but does not get correctly converted
         * to qml domain -> convert to variant list */
        QByteArray arr = val.toByteArray();
        QVariantList lst;
        for( int i = 0; i < arr.size(); ++i )
            lst <<QVariant::fromValue(static_cast<quint8>(arr[i]));
        res = QVariant::fromValue(lst);
    }
    else if (type == val.type()) {
        /* Already is built-in qt type, use as is */
        res = val;
    } else if (type == qMetaTypeId<QDBusVariant>()) {
        /* Convert QDBusVariant to QVariant */
        res = unwind(val.value<QDBusVariant>().variant(), depth);
    } else if (type == qMetaTypeId<QDBusObjectPath>()) {
        /* Convert QDBusObjectPath to QString */
        res = val.value<QDBusObjectPath>().path();
    } else if (type == qMetaTypeId<QDBusSignature>()) {
        /* Convert QDBusSignature to QString */
        res =  val.value<QDBusSignature>().signature();
    } else if (type == qMetaTypeId<QDBusUnixFileDescriptor>()) {
        /* Convert QDBusUnixFileDescriptor to int */
        res =  val.value<QDBusUnixFileDescriptor>().fileDescriptor();
    } else if (type == qMetaTypeId<QDBusArgument>()) {
        /* Try to deal with everything QDBusArgument could be ... */
        const QDBusArgument &arg = val.value<QDBusArgument>();
        const QDBusArgument::ElementType elem = arg.currentType();
        switch (elem) {
        case QDBusArgument::BasicType:
            /* Most of the basic types should be convertible to QVariant.
             * Recurse anyway to deal with object paths and the like. */
            res = unwind(arg.asVariant(), depth);
            break;

        case QDBusArgument::VariantType:
            /* Try to convert to QVariant. Recurse to check content */
            res = unwind(arg.asVariant().value<QDBusVariant>().variant(),
                         depth);
            break;

        case QDBusArgument::ArrayType:
            /* Convert dbus array to QVariantList */
            {
                QVariantList list;
                arg.beginArray();
                while (!arg.atEnd()) {
                    QVariant tmp = arg.asVariant();
                    list.append(unwind(tmp, depth));
                }
                arg.endArray();
                res = list;
            }
            break;

        case QDBusArgument::StructureType:
            /* Convert dbus struct to QVariantList */
            {
                QVariantList list;
                arg.beginStructure();
                while (!arg.atEnd()) {
                    QVariant tmp = arg.asVariant();
                    list.append(unwind(tmp, depth));
                }
                arg.endStructure();
                res = QVariant::fromValue(list);
            }
            break;

        case QDBusArgument::MapType:
            /* Convert dbus dict to QVariantMap */
            {
                QVariantMap map;
                arg.beginMap();
                while (!arg.atEnd()) {
                    arg.beginMapEntry();
                    QVariant key = arg.asVariant();
                    QVariant val = arg.asVariant();
                    map.insert(unwind(key, depth).toString(),
                               unwind(val, depth));
                    arg.endMapEntry();
                }
                arg.endMap();
                res = map;
            }
            break;

        default:
            /* Unhandled types produce invalid QVariant */
            qWarning() << "Unhandled QDBusArgument element type:" << elem;
            break;
        }
    } else {
        /* Default to using as is. This should leave for example QDBusError
         * types in a form that does not look like a string to qml code. */
        res = val;
        qWarning() << "Unhandled QVariant userType:" << type;
    }

    return res;
}
