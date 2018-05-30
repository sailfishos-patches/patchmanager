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

PatchManager::PatchManager(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_installedModel(new PatchManagerModel(this))
    , m_interface(new PatchManagerInterface(DBUS_SERVICE_NAME, DBUS_PATH_NAME, QDBusConnection::systemBus(), this))
{

    requestListPatches(QString(), false);
    connect(m_interface, &PatchManagerInterface::patchAltered, this, &PatchManager::requestListPatches);
    connect(m_interface, &PatchManagerInterface::updatesAvailable, this, &PatchManager::onUpdatesAvailable);
    connect(m_interface, &PatchManagerInterface::toggleServicesChanged, this, &PatchManager::onToggleServicesChanged);

    QDBusPendingCallWatcher *watchGetUpdates = new QDBusPendingCallWatcher(m_interface->getUpdates(), this);
    connect(watchGetUpdates, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            return;
        }

        qDebug() << reply.value();

        const QVariantMap data = PatchManager::unwind(reply.value()).toMap();
        onUpdatesAvailable(data);

        watcher->deleteLater();
    });

    QDBusPendingCallWatcher *watchGetToggleServices = new QDBusPendingCallWatcher(m_interface->getToggleServices(), this);
    connect(watchGetToggleServices, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            return;
        }

        qDebug() << reply.value();

        const bool toggleServices = reply.value();
        onToggleServicesChanged(toggleServices);

        watcher->deleteLater();
    });
}

PatchManager *PatchManager::GetInstance(QObject *parent)
{
    static PatchManager* lsSingleton = nullptr;
    if (!lsSingleton) {
        lsSingleton = new PatchManager(parent);
    }
    return lsSingleton;
}

QString PatchManager::serverMediaUrl()
{
    return QStringLiteral(MEDIA_URL);
}

bool PatchManager::developerMode()
{
    return getSettingsSync("developerMode", false).toBool();
}

void PatchManager::setDeveloperMode(bool developerMode)
{
    if (putSettingsSync("developerMode", developerMode)) {
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

QVariantMap PatchManager::getUpdates() const
{
    return m_updates;
}

QStringList PatchManager::getUpdatesNames() const
{
    return m_updates.keys();
}

bool PatchManager::toggleServices() const
{
    return m_toggleServices;
}

void PatchManager::call(QDBusPendingCallWatcher *call)
{
    connect(call,
            &QDBusPendingCallWatcher::finished,
            [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
    });
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
        const QVariantList data = PatchManager::unwind(reply.value()).toList();
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

QDBusPendingCallWatcher *PatchManager::downloadCatalog(const QVariantMap &params)
{
    return new QDBusPendingCallWatcher(m_interface->downloadCatalog(params), this);
}

QDBusPendingCallWatcher *PatchManager::downloadPatchInfo(const QString &name)
{
    return new QDBusPendingCallWatcher(m_interface->downloadPatchInfo(name), this);
}

QDBusPendingCallWatcher *PatchManager::listVersions()
{
    return new QDBusPendingCallWatcher(m_interface->listVersions(), this);
}

QDBusPendingCallWatcher *PatchManager::restartServices()
{
    return new QDBusPendingCallWatcher(m_interface->restartServices(), this);
}

//QDBusPendingCallWatcher *PatchManager::putSettings(const QString &name, const QVariant &value)
//{
//    return new QDBusPendingCallWatcher(m_interface->putSettings(name, value), this);
//}

//QDBusPendingCallWatcher *PatchManager::getSettings(const QString &name, const QVariant &def)
//{
//    return new QDBusPendingCallWatcher(m_interface->getSettings(name, def), this);
//}

void PatchManager::watchCall(QDBusPendingCallWatcher *call, QJSValue callback, QJSValue errorCallback)
{
    connect(call,
            &QDBusPendingCallWatcher::finished,
            [callback, errorCallback](QDBusPendingCallWatcher *watcher) mutable {
        QDBusPendingReply<> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            if (errorCallback.isCallable()) {
                QJSValueList callbackArguments;
                callbackArguments << QJSValue(reply.error().message());
                errorCallback.call(callbackArguments);
            }
        } else {
            if (callback.isCallable()) {
                const QDBusMessage message = reply.reply();
                const QVariantList arguments = PatchManager::unwind(message.arguments()).toList();
                QJSValueList callbackArguments;
                for (const QVariant &argument : arguments) {
                    callbackArguments << callback.engine()->toScriptValue<QVariant>(argument);
                }
                callback.call(callbackArguments);
            }
        }
        watcher->deleteLater();
    });
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

int PatchManager::checkVote(const QString &patch)
{
    return getSettingsSync(QStringLiteral("votes/%1").arg(patch), 0).toInt();
}

void PatchManager::doVote(const QString &patch, int action)
{
    qDebug() << Q_FUNC_INFO << patch << action;

    if (checkVote(patch) == action) {
        return;
    }

    m_interface->votePatch(patch, action);

    putSettingsSync(QStringLiteral("votes/%1").arg(patch), action);
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

void PatchManager::checkForUpdates()
{
    m_interface->checkForUpdates();
}

bool PatchManager::putSettingsSync(const QString &name, const QVariant &value)
{
    QDBusPendingReply<bool> reply = m_interface->putSettings(name, QDBusVariant(value));
    reply.waitForFinished();
    if (reply.isFinished()) {
        return reply.value();
    }
    return false;
}

void PatchManager::putSettingsAsync(const QString &name, const QVariant &value, QJSValue callback, QJSValue errorCallback)
{
    watchCall(new QDBusPendingCallWatcher(m_interface->putSettings(name, QDBusVariant(value)), this), callback, errorCallback);
}

QVariant PatchManager::getSettingsSync(const QString &name, const QVariant &def)
{
    QDBusPendingReply<QVariant> reply = m_interface->getSettings(name, QDBusVariant(def));
    reply.waitForFinished();
    if (reply.isFinished()) {
        return PatchManager::unwind(reply.value());
    }
    return QVariant();
}

void PatchManager::getSettingsAsync(const QString &name, const QVariant &def, QJSValue callback, QJSValue errorCallback)
{
    watchCall(new QDBusPendingCallWatcher(m_interface->getSettings(name, QDBusVariant(def)), this), callback, errorCallback);
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

void PatchManager::onUpdatesAvailable(const QVariantMap &updates)
{
    if (m_updates == updates) {
        return;
    }

    qDebug() << Q_FUNC_INFO << updates;

    m_updates = updates;
    emit updatesChanged();
}

void PatchManager::onToggleServicesChanged(bool toggle)
{
    if (m_toggleServices == toggle) {
        return;
    }

    m_toggleServices = toggle;
    emit toggleServicesChanged(m_toggleServices);
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
            list.append(PatchManager::unwind(var, depth));
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
        res = PatchManager::unwind(val.value<QDBusVariant>().variant(), depth);
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
            res = PatchManager::unwind(arg.asVariant(), depth);
            break;

        case QDBusArgument::VariantType:
            /* Try to convert to QVariant. Recurse to check content */
            res = PatchManager::unwind(arg.asVariant().value<QDBusVariant>().variant(),
                         depth);
            break;

        case QDBusArgument::ArrayType:
            /* Convert dbus array to QVariantList */
            {
                QVariantList list;
                arg.beginArray();
                while (!arg.atEnd()) {
                    QVariant tmp = arg.asVariant();
                    list.append(PatchManager::unwind(tmp, depth));
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
                    list.append(PatchManager::unwind(tmp, depth));
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
                    map.insert(PatchManager::unwind(key, depth).toString(),
                               PatchManager::unwind(val, depth));
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
