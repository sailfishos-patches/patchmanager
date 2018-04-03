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

#include "patchmanagerobject.h"
#include "patchmanager_adaptor.h"

#include <QLocalSocket>
#include <QLocalServer>

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>
#include <QtCore/QVector>

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusVariant>
#include <QtDBus/QDBusConnectionInterface>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <nemonotifications-qt5/notification.h>
#include "inotifywatcher.h"

#include <unistd.h>
#include <sys/stat.h>

#define NAME(x) #x

#define DBUS_GUARD(x) \
if (!calledFromDBus()) {\
    qWarning() << Q_FUNC_INFO << "This function should be only called from D-Bus!";\
    return x;\
}

static const QString PATCHES_DIR = QStringLiteral("/usr/share/patchmanager/patches");
static const QString PATCHES_ADDITIONAL_DIR = QStringLiteral("/var/lib/patchmanager/ausmt/patches");
static const QString PATCH_FILE = QStringLiteral("patch.json");

static const QString NAME_KEY = QStringLiteral("name");
static const QString DESCRIPTION_KEY = QStringLiteral("description");
static const QString CATEGORY_KEY = QStringLiteral("category");
static const QString INFOS_KEY = QStringLiteral("infos");
static const QString PATCH_KEY = QStringLiteral("patch");
static const QString RPM_KEY = QStringLiteral("rpm");
static const QString AVAILABLE_KEY = QStringLiteral("available");
static const QString SECTION_KEY = QStringLiteral("section");
static const QString PATCHED_KEY = QStringLiteral("patched");
static const QString VERSION_KEY = QStringLiteral("version");
static const QString COMPATIBLE_KEY = QStringLiteral("compatible");
static const QString ISCOMPATIBLE_KEY = QStringLiteral("isCompatible");
static const QString CONFLICTS_KEY = QStringLiteral("conflicts");

static const QString AUSMT_INSTALLED_LIST_FILE = QStringLiteral("/var/lib/patchmanager/ausmt/packages");
static const QString AUSMT_INSTALL = QStringLiteral("/opt/ausmt/ausmt-install");
static const QString AUSMT_REMOVE = QStringLiteral("/opt/ausmt/ausmt-remove");

static const QString BROWSER_CODE = QStringLiteral("browser");
static const QString CAMERA_CODE = QStringLiteral("camera");
static const QString CALENDAR_CODE = QStringLiteral("calendar");
static const QString CLOCK_CODE = QStringLiteral("clock");
static const QString CONTACTS_CODE = QStringLiteral("contacts");
static const QString EMAIL_CODE = QStringLiteral("email");
static const QString GALLERY_CODE = QStringLiteral("gallery");
static const QString HOMESCREEN_CODE = QStringLiteral("homescreen");
static const QString MEDIA_CODE = QStringLiteral("media");
static const QString MESSAGES_CODE = QStringLiteral("messages");
static const QString PHONE_CODE = QStringLiteral("phone");
static const QString SILICA_CODE = QStringLiteral("silica");
static const QString SETTINGS_CODE = QStringLiteral("settings");

static const QString newConfigLocation = QStringLiteral("/etc/patchmanager2.conf");
static const QString oldConfigLocation = QStringLiteral("/home/nemo/.config/patchmanager2.conf");

static const QString patchmanager_socket = QStringLiteral("/tmp/patchmanager-socket");
static const QString patchmanager_cache_root = QStringLiteral("/tmp/patchmanager");

static bool patchSort(const QVariantMap &patch1, const QVariantMap &patch2)
{
    if (patch1[CATEGORY_KEY].toString() == patch2[CATEGORY_KEY].toString()) {
        return patch1[NAME_KEY].toString() < patch2[NAME_KEY].toString();
    }

    return patch1[CATEGORY_KEY].toString() < patch2[CATEGORY_KEY].toString();
}

bool PatchManagerObject::makePatch(const QDir &root, const QString &patchPath, QVariantMap &patch, bool available)
{
    QDir patchDir(root);
    if (!patchDir.cd(patchPath)) {
        return false;
    }

    QFile file(patchDir.absoluteFilePath(PATCH_FILE));
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

    json[PATCH_KEY] = patchPath;
    json[RPM_KEY] = checkRpmPatch(patchPath);
    json[AVAILABLE_KEY] = available;
//    json[CATEGORY_KEY] = json[CATEGORY_KEY];
//    json[SECTION_KEY] = QStringLiteral("Other");
    json[PATCHED_KEY] = m_appliedPatches.contains(patchPath);
    if (!json.contains(VERSION_KEY)) {
        json[VERSION_KEY] = QStringLiteral("0.0.0");
    }
    if (!json.contains(COMPATIBLE_KEY)) {
        json[COMPATIBLE_KEY] = QStringList();
        json[ISCOMPATIBLE_KEY] = true;
    } else {
        json[ISCOMPATIBLE_KEY] = json[COMPATIBLE_KEY].toStringList().contains(m_ssuRelease);
    }
    json[CONFLICTS_KEY] = QStringList();
    patch = json;

    return true;
}

void PatchManagerObject::notify(const QString &patch, bool apply, bool success)
{
    QString summary;
    QString body;

    if (apply && success) {
        // Installing
        summary = qApp->translate("", "Patch installed");
        body = qApp->translate("", "Patch %1 installed").arg(patch);
    } else if (apply) {
        summary = qApp->translate("", "Failed to install patch");
        body = qApp->translate("", "Patch %1 installation failed").arg(patch);
    } else if (success) {
        // Removing
        summary = qApp->translate("", "Patch removed");
        body = qApp->translate("", "Patch %1 removed").arg(patch);
    } else {
        summary = qApp->translate("", "Failed to remove patch");
        body = qApp->translate("", "Patch %1 removal failed").arg(patch);
    }

    Notification notification;
    notification.setAppName(qApp->translate("", "Patchmanager"));
    notification.setHintValue("x-nemo-icon", "icon-m-patchmanager2");
    notification.setHintValue("x-nemo-preview-icon", "icon-m-patchmanager2");
    notification.setSummary(summary);
    notification.setBody(body);
    notification.setPreviewSummary(summary);
    notification.setPreviewBody(body);
    notification.setTimestamp(QDateTime::currentDateTime());
    notification.publish();
}

void PatchManagerObject::getVersion()
{
    qDebug() << Q_FUNC_INFO;
    QDBusMessage msg = QDBusMessage::createMethodCall("org.nemo.ssu", "/org/nemo/ssu", "org.nemo.ssu", "release");
    msg.setArguments({ QVariant::fromValue(false) });
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        if (!watcher->isError()) {
            m_ssuRelease = QDBusPendingReply<QString>(*watcher);
        }
        watcher->deleteLater();
    });
}

QList<QVariantMap> PatchManagerObject::listPatchesFromDir(const QString &dir, QSet<QString> &existingPatches, bool existing)
{
    QList<QVariantMap> patches;
    QDir root (dir);
    for (const QString &patchPath : root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
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

PatchManagerObject::PatchManagerObject(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_nam(new QNetworkAccessManager(this))
    , m_originalWatcher(new QFileSystemWatcher(this))
    , m_settings(new QSettings(newConfigLocation, QSettings::IniFormat, this))
    , m_localServer(new QLocalServer(this))
{
    if (!QFileInfo::exists(newConfigLocation) && QFileInfo::exists(oldConfigLocation)) {
        QFile::copy(oldConfigLocation, newConfigLocation);
    }

    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        installEventFilter(this);
    }

    if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS")) {
        qWarning() << "Session bus address is not set! Please check environment configuration!";
    }

    connect(m_timer, &QTimer::timeout, this, &PatchManagerObject::onTimerAction);
    m_timer->setSingleShot(false);
    m_timer->setTimerType(Qt::VeryCoarseTimer);
    m_timer->setInterval(15000);
    m_timer->start();

    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/org/freedesktop/systemd1/unit/lipstick_2eservice"),
                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                          QStringLiteral("PropertiesChanged"), this, SLOT(onLipstickChanged(QString,QVariantMap,QStringList)));

    connect(m_originalWatcher, &QFileSystemWatcher::fileChanged, this, &PatchManagerObject::onOriginalFileChanged);

    m_localServer->setSocketOptions(QLocalServer::WorldAccessOption);
    m_localServer->setMaxPendingConnections(2147483647);
    connect(m_localServer, &QLocalServer::newConnection, this, &PatchManagerObject::startReadingLocalServer);
    bool listening = m_localServer->listen(patchmanager_socket);
    if (!listening // not listening
            && m_localServer->serverError() == QAbstractSocket::AddressInUseError // because have error which is matching AddressInUseError
            && QFileInfo::exists(patchmanager_socket) // socket file already exists
            && QFile::remove(patchmanager_socket)) { // and successfully removed it
        qWarning() << Q_FUNC_INFO << "Removed old stuck socket";
        listening = m_localServer->listen(patchmanager_socket); // try to start lisening again
    }
    qDebug() << Q_FUNC_INFO << "Server listening:" << listening;
}

PatchManagerObject::~PatchManagerObject()
{
    if (m_dbusRegistered) {
        QDBusConnection connection = QDBusConnection::systemBus();
        connection.unregisterObject(DBUS_PATH_NAME);
        connection.unregisterService(DBUS_SERVICE_NAME);
    }
}

void PatchManagerObject::registerDBus()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doRegisterDBus), Qt::QueuedConnection);
}

void PatchManagerObject::doRegisterDBus()
{
    qDebug() << Q_FUNC_INFO;
    if (m_dbusRegistered) {
        return;
    }

    QDBusConnection connection = QDBusConnection::systemBus();

    if (connection.interface()->isServiceRegistered(DBUS_SERVICE_NAME)) {
        qWarning() << "Service already registered:" << DBUS_SERVICE_NAME;
        return;
    }

    if (!connection.registerObject(DBUS_PATH_NAME, this)) {
        qCritical() << "Cannot register object:" << DBUS_PATH_NAME;
        QCoreApplication::quit();
        return;
    }

    qWarning() << "Object registered:" << DBUS_PATH_NAME;

    if (!connection.registerService(DBUS_SERVICE_NAME)) {
        qCritical() << "Cannot register D-Bus service:" << DBUS_SERVICE_NAME;
        QCoreApplication::quit();
        return;
    }

    m_adaptor = new PatchManagerAdaptor(this);
    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        m_adaptor->installEventFilter(this);
    }
    qWarning() << "Service registered:" << DBUS_SERVICE_NAME;
    m_dbusRegistered = true;
}

void PatchManagerObject::doPrepareCacheRoot()
{
    qWarning() << Q_FUNC_INFO;
    // TODO: think about security issues here
    for (const QString &patchName : m_appliedPatches) {
        doPatch(patchName, true);
    }
}

void PatchManagerObject::doPrepareCache(const QString &patchName, bool apply)
{
    qWarning() << Q_FUNC_INFO << patchName << apply;

    if (!m_patchFiles.contains(patchName)) {
        qWarning() << Q_FUNC_INFO << "Not installed:" << patchName;
        return;
    }

    for (const QString &fileName : m_patchFiles.value(patchName)) {
        QFileInfo fi(fileName);

        if (!fi.exists()) {
            qWarning() << Q_FUNC_INFO << "warning!" << fileName << "does not exists!";
            continue; // TODO: WTF?
        }
        QDir fakeDir(QStringLiteral("%1%2").arg(patchmanager_cache_root, fi.absoluteDir().absolutePath()));
        if (!fakeDir.exists()) {
            QDir::root().mkpath(fakeDir.absolutePath());
        }

        const QString fakeFileName = QStringLiteral("%1/%2").arg(fakeDir.absolutePath(), fi.fileName());

        if (QFileInfo::exists(fakeFileName)) {
            if (apply) {
                m_originalWatcher->addPath(fileName);
                continue;
            }

            if (m_fileToPatch.value(fileName).length() > 1) {
                continue;
            }

            m_originalWatcher->removePath(fileName);
            QFile::remove(fakeFileName);
        } else {
            if (!apply) {
                continue;
            }

            struct stat fileStat;
            if (stat(fileName.toLatin1().constData(), &fileStat) < 0) {
                qWarning() << Q_FUNC_INFO << "warning!" << fileName << "could not be stat!";
                continue;
            }

            qWarning() << Q_FUNC_INFO << "Copying" << fileName << "to" << fakeFileName;
            m_originalWatcher->addPath(fileName);
            QFile::copy(fileName, fakeFileName);

            chmod(fakeFileName.toLatin1().constData(), fileStat.st_mode);
            chown(fakeFileName.toLatin1().constData(), fileStat.st_uid, fileStat.st_gid);
        }
    }
 }

void PatchManagerObject::initialize()
{
    getVersion();
    refreshPatchList();
    prepareCacheRoot();

    INotifyWatcher *mainWatcher = new INotifyWatcher(this);
    mainWatcher->addPaths({ PATCHES_DIR });
    connect(mainWatcher, &INotifyWatcher::contentChanged, [this](const QString &path, bool created) {
        qDebug() << "contentChanged:" << path << "created:" << created;
        refreshPatchList();
        if (m_adaptor) {
            emit m_adaptor->patchAltered(path, created);
        }
    });

//    INotifyWatcher *additionalWatcher = new INotifyWatcher(this);
//    additionalWatcher->addPaths({ PATCHES_ADDITIONAL_DIR });
//    connect(additionalWatcher, &INotifyWatcher::contentChanged, [this](const QString &path, bool created) {
//        qDebug() << "contentChanged:" << path << "created:" << created;
//        refreshPatchList();
    //    });
}

QString PatchManagerObject::checkRpmPatch(const QString &patch) const
{
    QString patchPath = QStringLiteral("/usr/share/patchmanager/patches/%1/unified_diff.patch").arg(patch);
    if (!QFile(patchPath).exists()) {
        return QString();
    }
    QProcess proc;
    proc.start(QStringLiteral("/bin/rpm"), { QStringLiteral("-qf"), QStringLiteral("--qf"), QStringLiteral("%{NAME}"), patchPath });
    if (!proc.waitForFinished(5000) || proc.exitCode() != 0) {
        return QString();
    }
    const QString package = QString::fromLatin1(proc.readAllStandardOutput());
    if (package.isEmpty()) {
        return QString();
    }
    return package;
}

void PatchManagerObject::process()
{
    qDebug() << Q_FUNC_INFO;
    const QStringList args = QCoreApplication::arguments();

    QDBusConnection connection = QDBusConnection::systemBus();
    if (connection.interface()->isServiceRegistered(DBUS_SERVICE_NAME)) {
        QString method;
        QVariantList data;
        if (args[1] == QStringLiteral("-a")) {
            method = QStringLiteral("applyPatch");
            if (args.length() < 3) {
                QCoreApplication::exit(2);
                return;
            } else {
                data.append(args[2]);
            }
        } else if (args[1] == QStringLiteral("-u")) {
            method = QStringLiteral("unapplyPatch");
            if (args.length() < 3) {
                QCoreApplication::exit(2);
                return;
            } else {
                data.append(args[2]);
            }
        } else if (args[1] == QStringLiteral("--unapply-all")) {
            method = QStringLiteral("unapplyAllPatches");
        } else {
            return;
        }

        QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE_NAME, DBUS_PATH_NAME, DBUS_SERVICE_NAME, method);
        if (!data.isEmpty()) {
            msg.setArguments(data);
        }
        connection.send(msg);

        QCoreApplication::exit(2);
        return;
    }

    initialize();
    registerDBus();

    if (args[1] == QStringLiteral("-a")) {
        if (args.length() < 3) {
            return;
        } else {
            applyPatch(args[2]);
        }
    } else if (args[1] == QStringLiteral("-u")) {
        if (args.length() < 3) {
            return;
        } else {
            unapplyPatch(args[2]);
        }
    } else if (args[1] == QStringLiteral("--unapply-all")) {
        unapplyAllPatches();
    }
}

QVariantList PatchManagerObject::listPatches()
{
    DBUS_GUARD(QVariantList())
    qDebug() << Q_FUNC_INFO;
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, NAME(doListPatches), Qt::QueuedConnection, Q_ARG(QDBusMessage, message()));
    return QVariantList();
}

QVariantMap PatchManagerObject::listVersions()
{
    qDebug() << Q_FUNC_INFO;
    QVariantMap versionsList;
    for (const QString &patch : m_metadata.keys()) {
        versionsList[patch] = m_metadata[patch][VERSION_KEY];
    }

    return versionsList;
}

bool PatchManagerObject::isPatchApplied(const QString &patch)
{
    qDebug() << Q_FUNC_INFO;
    return m_appliedPatches.contains(patch);
}

bool PatchManagerObject::applyPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);
    QDBusMessage msg;
    if (calledFromDBus()) {
        msg = message();
    }
    QMetaObject::invokeMethod(this, NAME(doPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{QStringLiteral("name"), patch}})),
                              Q_ARG(QDBusMessage, message()),
                              Q_ARG(bool, true));
    return true;

    QVariantMap patchData = m_metadata[patch];
    QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

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
    notify(displayName.toString(), true, ok);

    if (ok) {
//        emit m_adaptor->applyPatchFinished(patch);
    }
    return ok;
}

bool PatchManagerObject::unapplyPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);
    QDBusMessage msg;
    if (calledFromDBus()) {
        msg = message();
    }
    QMetaObject::invokeMethod(this, NAME(doPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{QStringLiteral("name"), patch}})),
                              Q_ARG(QDBusMessage, message()),
                              Q_ARG(bool, false));
    return true;

    QVariantMap patchData = m_metadata[patch];
    QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

    QProcess process;
    process.setProgram(AUSMT_REMOVE);

    QStringList arguments;
    arguments.append(patch);

    process.setArguments(arguments);
    process.start();
    process.waitForFinished(-1);

    bool ok = (process.exitCode() == 0);
    qDebug() << "ok:" << ok;
    if (ok) {
        m_appliedPatches.remove(patch);
        refreshPatchList();
    }
    notify(displayName.toString(), false, ok);

    if (ok) {
//        emit m_adaptor->unapplyPatchFinished(patch);
    }
    return ok;
}

bool PatchManagerObject::unapplyAllPatches()
{
    qDebug() << Q_FUNC_INFO;
    return true;

    bool ok = true;
    for (const QString &patch : m_appliedPatches.toList()) {
        ok &= unapplyPatch(patch);
    }
//    emit m_adaptor->unapplyAllPatchesFinished();
    return ok;
}

bool PatchManagerObject::installPatch(const QString &patch, const QString &version, const QString &url)
{
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, NAME(doInstallPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{"patch", patch}, {"version", version}, {"url", url}})),
                              Q_ARG(QDBusMessage, message()));
    return true;
}

bool PatchManagerObject::uninstallPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);

    if (m_appliedPatches.contains(patch)) {
        unapplyPatch(patch);
    }
    QMetaObject::invokeMethod(this, NAME(doUninstallPatch), Qt::QueuedConnection,
                              Q_ARG(QString, patch),
                              Q_ARG(QDBusMessage, message()));
    return true;
}

bool PatchManagerObject::resetPatchState(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);

    if (m_appliedPatches.contains(patch)) {
        unapplyPatch(patch);
    }
    QMetaObject::invokeMethod(this, NAME(doResetPatchState), Qt::QueuedConnection,
                              Q_ARG(QString, patch),
                              Q_ARG(QDBusMessage, message()));
    return true;
}

int PatchManagerObject::checkVote(const QString &patch)
{
    DBUS_GUARD(0)
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, NAME(doCheckVote), Qt::QueuedConnection,
                              Q_ARG(QString, patch),
                              Q_ARG(QDBusMessage, message()));
    return 0;
}

void PatchManagerObject::votePatch(const QString &patch, int action)
{
    qDebug() << Q_FUNC_INFO << patch << action;
    QMetaObject::invokeMethod(this, NAME(sendVote), Qt::QueuedConnection,
                              Q_ARG(QString, patch),
                              Q_ARG(int, action));
}

QString PatchManagerObject::checkEaster()
{
    DBUS_GUARD(QString())
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doCheckEaster), Qt::QueuedConnection,
                              Q_ARG(QDBusMessage, message()));
    return QString();
}

QVariantList PatchManagerObject::downloadCatalog(const QVariantMap &params)
{
    DBUS_GUARD(QVariantList())
    qDebug() << Q_FUNC_INFO << params;
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, NAME(requestDownloadCatalog), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, params),
                              Q_ARG(QDBusMessage, message()));
    return QVariantList();
}

QVariantMap PatchManagerObject::downloadPatchInfo(const QString &name)
{
    DBUS_GUARD(QVariantMap())
    qDebug() << Q_FUNC_INFO << name;
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, NAME(requestDownloadPatchInfo), Qt::QueuedConnection,
                              Q_ARG(QString, name),
                              Q_ARG(QDBusMessage, message()));
    return QVariantMap();
}

void PatchManagerObject::checkForUpdates()
{
    DBUS_GUARD()
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(requestCheckForUpdates), Qt::QueuedConnection);
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

bool PatchManagerObject::eventFilter(QObject *watched, QEvent *event)
{
    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        qDebug() << watched << event->type();
    }
    return QObject::eventFilter(watched, event);
}

void PatchManagerObject::onLipstickChanged(const QString &, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (invalidatedProperties.contains(QStringLiteral("ActiveState"))) {
        return;
    }

    const QString activeState = changedProperties.value(QStringLiteral("ActiveState"), QStringLiteral("unknown")).toString();
    if (activeState == QStringLiteral("failed")) {
        qWarning() << "Detected lipstick crash, deactivating all patches";
        unapplyAllPatches();
    }
}

void PatchManagerObject::onTimerAction()
{

}

void PatchManagerObject::startReadingLocalServer()
{
    QLocalSocket *clientConnection = m_localServer->nextPendingConnection();
    if (!clientConnection) {
        qWarning() << Q_FUNC_INFO << "Got empty connection!";
        return;
    }
    qDebug() << Q_FUNC_INFO << "Got new connection" << clientConnection << clientConnection->state();
    if (clientConnection->state() != QLocalSocket::ConnectedState) {
        qWarning() << "Socket client is not connected yet!";
        clientConnection->waitForConnected();
    }
    connect(clientConnection, &QLocalSocket::disconnected, [clientConnection](){
        qDebug() << "Client disconnected:" << clientConnection;
        clientConnection->deleteLater();
    });
    connect(clientConnection, &QLocalSocket::readyRead, this, &PatchManagerObject::readFromLocalClient);
}

void PatchManagerObject::readFromLocalClient()
{
    QLocalSocket *clientConnection = qobject_cast<QLocalSocket*>(sender());
    if (!clientConnection) {
        return;
    }
    if (clientConnection->bytesAvailable() <= 0) {
        qWarning() << Q_FUNC_INFO << "Nothing to read";
    }
    QByteArray payload = clientConnection->readAll();
    qWarning() << Q_FUNC_INFO << "Requested:" << payload;
    const QString fakePath = QStringLiteral("%1%2").arg(patchmanager_cache_root, QString::fromLatin1(payload));
    if (QFileInfo::exists(fakePath)) {
        payload = fakePath.toLatin1();
    }
    qWarning() << Q_FUNC_INFO << "Sending:" << payload;
    clientConnection->write(payload);
    clientConnection->flush();
//    clientConnection->waitForBytesWritten();
}

void PatchManagerObject::onOriginalFileChanged(const QString &path)
{
    qWarning() << Q_FUNC_INFO << path;

    if (!m_fileToPatch.contains(path)) {
        return;
    }

    QStringList patches = m_fileToPatch[path];
    for (const QString &patch : patches) {
        if (!m_appliedPatches.contains(patch)) {
            patches.removeAll(patch);
        }
    }

    for (const QString &patch : patches) {
        doPatch(patch, false);
    }

    for (const QString &patch : patches) {
        doPatch(patch, true);
    }
}

void PatchManagerObject::doRefreshPatchList()
{
    qDebug() << Q_FUNC_INFO;

    // load applied patches

    QFile file (AUSMT_INSTALLED_LIST_FILE);
    if (file.open(QFile::ReadOnly)) {
        while (!file.atEnd()) {
            const QString line = QString::fromLatin1(file.readLine());
            QStringList splitted = line.split(QChar(' '));
            if (splitted.count() == 2) {
                m_appliedPatches.insert(splitted.first());
                qDebug() << splitted.first();
            }
        }
        file.close();
    }

    // scan all patches
    // collect conflicts per file

    m_patchFiles.clear();
    m_fileToPatch.clear();
    QMap<QString, QStringList> filesConflicts;
    QDir patchesDir(PATCHES_DIR);
    for (const QString &patchFolder : patchesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile patchFile(QStringLiteral("%1/%2/unified_diff.patch").arg(PATCHES_DIR, patchFolder));
        if (!patchFile.exists() || !patchFile.open(QFile::ReadOnly)) {
            continue;
        }
        while (!patchFile.atEnd()) {
            QByteArray line = patchFile.readLine();
            if (line.startsWith(QByteArrayLiteral("+++ "))) {
                QString toPatch = QString::fromLatin1(line.split(' ')[1].split('\t')[0].split('\n')[0]);
                QString path = toPatch;
                while (!QFileInfo::exists(path) && path.count('/') > 1) {
                    path = path.mid(path.indexOf('/', 1));
                }
                if (!QFileInfo::exists(path)) {
                    if (toPatch.startsWith(QChar('/'))) {
                        path = toPatch;
                    } else {
                        path = toPatch.mid(toPatch.indexOf('/', 1));
                    }
                }
                filesConflicts[path].append(patchFolder);

                QStringList patchFiles = m_patchFiles[patchFolder];
                if (!patchFiles.contains(path)) {
                    patchFiles.append(path);
                }
                m_patchFiles[patchFolder] = patchFiles;

                QStringList fileToPatch = m_fileToPatch[path];
                if (!fileToPatch.contains(patchFolder)) {
                    fileToPatch.append(patchFolder);
                }
                m_fileToPatch[path] = fileToPatch;
            }
        }

        patchFile.close();
    }

    // collect conflicts per patch

    QMap<QString, QStringList> patchConflicts;
    for (const QStringList &conflictList : filesConflicts) {
        for (const QString &conflict : conflictList) {
            QStringList exclusiveConflicts = conflictList;
            exclusiveConflicts.removeOne(conflict);
            QStringList existingConflicts = patchConflicts[conflict];
            for (const QString &exclusiveConflict : exclusiveConflicts) {
                if (!existingConflicts.contains(exclusiveConflict)) {
                    existingConflicts.append(exclusiveConflict);
                }
            }
            patchConflicts[conflict] = existingConflicts;
        }
    }

    // get patches

    QSet<QString> existingPatches;
    QList<QVariantMap> patches = listPatchesFromDir(PATCHES_DIR, existingPatches);
    patches.append(listPatchesFromDir(PATCHES_ADDITIONAL_DIR, existingPatches, false));
//    std::sort(patches.begin(), patches.end(), patchSort);

    // fill patch conflicts

    m_metadata.clear();
    for (QVariantMap &patch : patches) {
        const QString patchName = patch[PATCH_KEY].toString();
        if (patchConflicts.contains(patchName)) {
            patch[CONFLICTS_KEY] = patchConflicts[patchName];
        }

        m_metadata[patchName] = patch;
    }

    QVariantMap debug;
    for (const QString &debugKey : m_metadata.keys()) {
        debug[debugKey] = m_metadata[debugKey];
    }

    qDebug().noquote() << QJsonDocument::fromVariant(debug).toJson(QJsonDocument::Indented);

    if (m_adaptor) {
        emit m_adaptor->listPatchesChanged();
    }
}

void PatchManagerObject::doListPatches(const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO;
    QVariantList result;
    for (const QVariantMap &patch : m_metadata) {
        result.append(patch);
    }
    sendMessageReply(message, result);
}

bool PatchManagerObject::doPatch(const QString &patchName, bool apply)
{
    qDebug() << Q_FUNC_INFO << patchName << apply;
    if (apply) {
        doPrepareCache(patchName, apply);
    }

    QProcess process;
    process.setProgram(apply ? AUSMT_INSTALL : AUSMT_REMOVE);

    QStringList arguments;
    arguments.append(patchName);

    process.setArguments(arguments);
    qDebug() << "Starting:" << process.program() << process.arguments();
    process.start();
    process.waitForFinished(-1);

    if (!apply) {
        doPrepareCache(patchName, apply);
    }

    return process.exitCode() == 0;
}

void PatchManagerObject::doPatch(const QVariantMap &params, const QDBusMessage &message, bool apply)
{
    qDebug() << Q_FUNC_INFO << params << apply;
    const QString &patch = params.value(QStringLiteral("name")).toString();

    QVariantMap patchData = m_metadata[patch];
    QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

    bool ok = doPatch(patch, apply);
    qDebug() << "ok:" << ok;
    if (ok) {
        if (apply) {
            m_appliedPatches.insert(patch);
        } else {
            m_appliedPatches.remove(patch);
        }
        refreshPatchList();
    }

    if (!params.value(QStringLiteral("dont_notify"), false).toBool()) {
        notify(displayName.toString(), apply, ok);
    }

    if (ok) {
//        emit m_adaptor->applyPatchFinished(patch);
    }

    if (message.isDelayedReply()) {
        sendMessageReply(message, ok);
    }

    return;

    if (apply) {
        QVariantMap patchData = m_metadata[patch];
        QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

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
        notify(displayName.toString(), true, ok);

        if (ok) {
    //        emit m_adaptor->applyPatchFinished(patch);
        }
    //    return ok;
    } else {
        QVariantMap patchData = m_metadata[patch];
        QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

        QProcess process;
        process.setProgram(AUSMT_REMOVE);

        QStringList arguments;
        arguments.append(patch);

        process.setArguments(arguments);
        process.start();
        process.waitForFinished(-1);

        bool ok = (process.exitCode() == 0);
        qDebug() << "ok:" << ok;
        if (ok) {
            m_appliedPatches.remove(patch);
            refreshPatchList();
        }
        notify(displayName.toString(), false, ok);

        if (ok) {
    //        emit m_adaptor->unapplyPatchFinished(patch);
        }
    //    return ok;
    }
}

void PatchManagerObject::doResetPatchState(const QString &patch, const QDBusMessage &message)
{
    sendMessageReply(message, m_appliedPatches.remove(patch));
}

void PatchManagerObject::doInstallPatch(const QVariantMap &params, const QDBusMessage &message)
{
    qWarning() << Q_FUNC_INFO << params;

    const QString &patch = params.value(QStringLiteral("patch")).toString();
    const QString &version = params.value(QStringLiteral("version")).toString();
    const QString &jsonUrl = QStringLiteral("%1/%2").arg(CATALOG_URL, PROJECT_PATH);

    QUrl url(jsonUrl);
    QUrlQuery query;
    query.addQueryItem("name", patch);
    query.addQueryItem("version", version);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply, message, params, this](){
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }
        if (reply->bytesAvailable() <= 0) {
            sendMessageError(message, QStringLiteral("Cannot get json"));
            return;
        }
        QByteArray json = reply->readAll();

        QJsonParseError error;
        QJsonDocument::fromJson(json, &error);

        if (error.error != QJsonParseError::NoError) {
            sendMessageError(message, QStringLiteral("Error parsing json"));
        }
        QVariantMap newParams = params;
        newParams.insert(QStringLiteral("json"), QString::fromUtf8(json));
        downloadPatchArchive(newParams, message);
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError networkError){
        qWarning() << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << "ignoring ssl error:" << sslError.errorString();
        }
        reply->ignoreSslErrors(errors);
    });
}

void PatchManagerObject::downloadPatchArchive(const QVariantMap &params, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << params;

    const QString &url = params.value(QStringLiteral("url")).toString();
    const QString &patch = params.value(QStringLiteral("patch")).toString();
    const QString &json = params.value(QStringLiteral("json")).toString();
    const QString &archive = QStringLiteral("/tmp/%1").arg(url.section(QChar('/'), -1));

    QFile *archiveFile = new QFile(archive, this);
    if (!archiveFile->open(QFile::WriteOnly)) {
        return;
    }

    QUrl webUrl(QString(MEDIA_URL"/%1").arg(url));
    QNetworkRequest request(webUrl);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply, message, patch, archiveFile, archive, json, this](){
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }
        if (!archiveFile) {
            sendMessageError(message, QStringLiteral("Lost in the wild"));
            return;
        }
        archiveFile->close();

        const QString patchPath = QStringLiteral("%1/%2").arg(PATCHES_DIR, patch);
        const QString jsonPath = QStringLiteral("%1/%2").arg(patchPath, PATCH_FILE);

        QDir patchDir(patchPath);
        if (patchDir.exists()) {
            patchDir.removeRecursively();
        }
        if (!archiveFile->exists() || !patchDir.mkpath(patchPath)) {
            archiveFile->deleteLater();
            sendMessageError(message, QStringLiteral("Error operating with files"));
            return;
        }
        QFile jsonFile(jsonPath);
        if (!jsonFile.open(QFile::WriteOnly)) {
            archiveFile->deleteLater();
            sendMessageError(message, QStringLiteral("Error saving json"));
            return;
        }

        jsonFile.write(json.toLatin1());
        jsonFile.close();

        QProcess proc;
        int ret = 0;
        if (archive.endsWith(QStringLiteral(".zip"))) {
            ret = proc.execute(QStringLiteral("/usr/bin/unzip"), {archive, QStringLiteral("-d"), patchPath});
        } else {
            ret = proc.execute(QStringLiteral("/bin/tar"), {QStringLiteral("xzf"), archive, QStringLiteral("-C"), patchPath});
        }
        if (ret == 0) {
            refreshPatchList();
        } else {
            patchDir.removeRecursively();
        }

        sendMessageReply(message, ret == 0);

        if (archiveFile->exists()) {
            archiveFile->remove();
        }
        archiveFile->deleteLater();
    });
    QObject::connect(reply, &QNetworkReply::readyRead, [archiveFile, reply](){
        if (!archiveFile || !archiveFile->isOpen()) {
            return;
        }
        archiveFile->write(reply->read(reply->bytesAvailable()));
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError networkError){
        qWarning() << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << "ignoring ssl error:" << sslError.errorString();
        }
        reply->ignoreSslErrors(errors);
    });
}

void PatchManagerObject::doUninstallPatch(const QString &patch, const QDBusMessage &message)
{
    bool removeSuccess = false;
    const QString rpmPatch = m_metadata[patch][RPM_KEY].toString();
    if (rpmPatch.isEmpty()) {
        QDir patchDir(QStringLiteral("%1/%2").arg(PATCHES_DIR, patch));
        if (patchDir.exists()) {
            removeSuccess = patchDir.removeRecursively();
        }
    } else {
        QDBusInterface iface("com.jolla.jollastore", "/StoreClient", "com.jolla.jollastore", QDBusConnection::sessionBus());
        iface.call(QDBus::NoBlock, "removePackage", rpmPatch, true);
        removeSuccess = true;
    }

    if (removeSuccess) {
        // TODO: gracefully update models
        refreshPatchList();
    }
    sendMessageReply(message, removeSuccess);
}

int PatchManagerObject::getVote(const QString &patch)
{
    return m_settings->value(QStringLiteral("votes/%1").arg(patch), 0).toInt();
}

void PatchManagerObject::doCheckVote(const QString &patch, const QDBusMessage &message)
{
    sendMessageReply(message, getVote(patch));
}

void PatchManagerObject::sendVote(const QString &patch, int action)
{
    if (getVote(patch) == action) {
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
    QNetworkReply *reply = m_nam->get(request);
    // TODO server should return new votes count
    //QObject::connect(reply, &QNetworkReply::finished, this, &PatchManager::onServerReplied);

    QString key = QString("votes/%1").arg(patch);
    m_settings->setValue(key, action);
    m_settings->sync();
}

void PatchManagerObject::doCheckEaster(const QDBusMessage &message)
{
    QUrl url(CATALOG_URL"/easter");
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, message](){
        const QByteArray json = reply->readAll();

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(json, &error);

        if (error.error == QJsonParseError::NoError) {
            const QJsonObject &object = document.object();
            if (object.value("status").toBool()) {
                sendMessageReply(message, object.value("text").toString());
            } else {
                sendMessageReply(message, QString());
            }
        } else {
            sendMessageError(message, error.errorString());
        }
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, message, reply](QNetworkReply::NetworkError) {
        sendMessageError(message, reply->errorString());
    });
}

void PatchManagerObject::sendActivation(const QString &patch, const QString &version)
{
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", patch);
    query.addQueryItem("version", version);
    query.addQueryItem("action", "activation");
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = m_nam->get(request);
    // TODO return current count of activations (or emit)
    //QObject::connect(reply, &QNetworkReply::finished, this, &PatchManager::onServerReplied);
}

void PatchManagerObject::downloadPatch(const QString &patch, const QUrl &url, const QString &file)
{
    QFile *f = new QFile(file, this);
    if (!f->open(QFile::WriteOnly)) {
        return;
    }
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [patch, f](){
        if (!f) {
            return;
        }
        f->close();
        f->deleteLater();
        // TODO emit download complete
    });
    QObject::connect(reply, &QNetworkReply::readyRead, [f, reply](){
        if (!f || !f->isOpen()) {
            return;
        }
        f->write(reply->read(reply->bytesAvailable()));
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError networkError){
        qWarning() << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << "ignoring ssl error:" << sslError.errorString();
        }
        reply->ignoreSslErrors(errors);
    });
}

void PatchManagerObject::requestDownloadCatalog(const QVariantMap &params, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << params;
    QUrl url(CATALOG_URL"/"PROJECTS_PATH);
    QUrlQuery query;
    for (const QString &key : params.keys()) {
        query.addQueryItem(key, params.value(key).toString());
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, message, reply]() {
        qDebug() << Q_FUNC_INFO << "Error:" << reply->error() << "Bytes:" << reply->bytesAvailable();
        if (reply->error() == QNetworkReply::NoError && reply->bytesAvailable()) {
            const QByteArray json = reply->readAll();

            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(json, &error);

            if (error.error == QJsonParseError::NoError) {
                sendMessageReply(message, document.toVariant());
            } else {
                sendMessageError(message, error.errorString());
            }
        }
        reply->deleteLater();
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, message, reply](QNetworkReply::NetworkError) {
        sendMessageError(message, reply->errorString());
    });
}

void PatchManagerObject::requestDownloadPatchInfo(const QString &name, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << name;
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), name);
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, message, reply]() {
        qDebug() << Q_FUNC_INFO << "Error:" << reply->error() << "Bytes:" << reply->bytesAvailable();
        if (reply->error() == QNetworkReply::NoError && reply->bytesAvailable()) {
            const QByteArray json = reply->readAll();

            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(json, &error);

            if (error.error == QJsonParseError::NoError) {
                sendMessageReply(message, document.toVariant());
            } else {
                sendMessageError(message, error.errorString());
            }
        }
        reply->deleteLater();
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, message, reply](QNetworkReply::NetworkError) {
        sendMessageError(message, reply->errorString());
    });
}

void PatchManagerObject::requestCheckForUpdates()
{
    qDebug() << Q_FUNC_INFO;

    //
}

void PatchManagerObject::sendMessageReply(const QDBusMessage &message, const QVariant &result)
{
    QDBusMessage replyMessage = message.createReply(result);
    QDBusConnection connection = QDBusConnection::systemBus();
    connection.send(replyMessage);
}

void PatchManagerObject::sendMessageError(const QDBusMessage &message, const QString &errorString)
{
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage replyError = message.createErrorReply(QDBusError::Other, errorString);
    connection.send(replyError);
}

void PatchManagerObject::refreshPatchList()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doRefreshPatchList), Qt::QueuedConnection);
}

void PatchManagerObject::prepareCacheRoot()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doPrepareCacheRoot), Qt::QueuedConnection);
}
