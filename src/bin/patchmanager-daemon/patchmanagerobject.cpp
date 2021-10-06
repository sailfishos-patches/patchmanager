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

#include <QProcessEnvironment>

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

#include <rpm/rpmlib.h>
#include <rpm/header.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmts.h>

#define NAME(x) #x

#define DBUS_GUARD(x) \
if (!calledFromDBus()) {\
    qWarning() << Q_FUNC_INFO << "This function should be only called from D-Bus!";\
    return x;\
}

static const QString PATCHES_DIR = QStringLiteral("/usr/share/patchmanager/patches");
static const QString PATCHES_WORK_DIR_PREFIX = QStringLiteral("/tmp/patchmanager3");
static const QString PATCHES_WORK_DIR = QStringLiteral("%1/%2").arg(PATCHES_WORK_DIR_PREFIX, "work");
static const QString PATCHES_ADDITIONAL_DIR = QStringLiteral("%1/%2").arg(PATCHES_WORK_DIR_PREFIX, "patches");
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

static const QString AUSMT_BACKUP_DIR = QStringLiteral("/var/lib/patchmanager/ausmt/patches");
static const QString AUSMT_INSTALLED_LIST_FILE = QStringLiteral("/var/lib/patchmanager/ausmt/packages");

static const QString PM_APPLY = QStringLiteral("/usr/libexec/pm_apply");
static const QString PM_UNAPPLY = QStringLiteral("/usr/libexec/pm_unapply");

// external binaries
static const QString BIN_UNZIP = QStringLiteral("/usr/bin/unzip");
static const QString BIN_TAR = QStringLiteral("/bin/tar");
static const QString BIN_PKCON = QStringLiteral("/usr/bin/pkcon");
static const QString BIN_SYSTEMCTL_U = QStringLiteral("/bin/systemctl-user");
static const QString BIN_RPM = QStringLiteral("/bin/rpm");

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
static const QString KEYBOARD_CODE = QStringLiteral("keyboard");

static const QString s_newConfigLocation = QStringLiteral("/etc/patchmanager2.conf");
static const QString s_oldConfigLocation = QStringLiteral("/home/nemo/.config/patchmanager2.conf");

static const QString s_patchmanagerSocket = QStringLiteral("/tmp/patchmanager-socket");
static const QString s_patchmanagerCacheRoot = QStringLiteral("/tmp/patchmanager");

static const QString s_sessionBusConnection = QStringLiteral("pm3connection");

QString getLang()
{
    QString lang = QStringLiteral("en_US.utf8");

    QStringList locales = {
        QStringLiteral("/etc/locale.conf"),
        QStringLiteral("/var/lib/environment/nemo/locale.conf"),
    };
    for (const QString &localePath : locales) {
        QFile localeConfig(localePath);

        if (!localeConfig.exists() || !localeConfig.open(QFile::ReadOnly)) {
            continue;
        }

        while (!localeConfig.atEnd()) {
            QString line = localeConfig.readLine().trimmed();
            if (line.startsWith(QStringLiteral("LANG="))) {
                 lang = line.mid(5);
                 break;
            }
        }
    }

    qDebug() << Q_FUNC_INFO << lang;

    return lang;
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
        qWarning() << Q_FUNC_INFO << error.errorString();
        return false;
    }

    QVariantMap json = document.toVariant().toMap();

    if (!json.contains(NAME_KEY) || !json.contains(DESCRIPTION_KEY) || !json.contains(CATEGORY_KEY)) {
        return false;
    }

    json[PATCH_KEY] = patchPath;
    json[RPM_KEY] = QString();
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
        json[ISCOMPATIBLE_KEY] = json[COMPATIBLE_KEY].toStringList().contains(m_osRelease);
    }
    json[CONFLICTS_KEY] = QStringList();
    patch = json;

    return true;
}

void PatchManagerObject::notify(const QString &patch, NotifyAction action)
{
    qDebug() << Q_FUNC_INFO << patch << action;

    Notification notification;
    notification.setAppName(qApp->translate("", "Patchmanager"));
    notification.setHintValue("app_icon", "icon-m-patchmanager2");
    notification.setTimestamp(QDateTime::currentDateTime());

    QString summary;
    QString body;
    QVariantList remoteActions;

    switch (action) {
    case NotifyActionSuccessApply:
        summary = qApp->translate("", "Patch installed");
        body = qApp->translate("", "Patch %1 installed").arg(patch);
        break;
    case NotifyActionSuccessUnapply:
        summary = qApp->translate("", "Patch removed");
        body = qApp->translate("", "Patch %1 removed").arg(patch);
        break;
    case NotifyActionFailedApply:
        summary = qApp->translate("", "Failed to install patch");
        body = qApp->translate("", "Patch %1 installation failed").arg(patch);
        break;
    case NotifyActionFailedUnapply:
        summary = qApp->translate("", "Failed to remove patch");
        body = qApp->translate("", "Patch %1 removal failed").arg(patch);
        break;
    case NotifyActionUpdateAvailable:
        summary = qApp->translate("", "Update available");
        body = qApp->translate("", "Patch %1 have update candidate").arg(patch);

        remoteActions << Notification::remoteAction(
            QStringLiteral("default"),
            QStringLiteral("patchmanager"),
            QStringLiteral("com.jolla.settings"),
            QStringLiteral("/com/jolla/settings/ui"),
            QStringLiteral("com.jolla.settings.ui"),
            QStringLiteral("showPage"),
            { QStringLiteral("system_settings/look_and_feel/patchmanager") }
        );
        break;
    default:
        qWarning() << Q_FUNC_INFO << "Unhandled action:" << action;
    }

    qDebug() << Q_FUNC_INFO << summary << body;

    if (!remoteActions.isEmpty()) {
        qDebug() << Q_FUNC_INFO << remoteActions;
        notification.setRemoteActions(remoteActions);
    }

    notification.setSummary(summary);
    notification.setBody(body);
    notification.setPreviewSummary(summary);
    notification.setPreviewBody(body);
    notification.publish();

    qDebug() << Q_FUNC_INFO << notification.replacesId();
}

QSet<QString> PatchManagerObject::getAppliedPatches() const
{
    return getSettings(QStringLiteral("applied"), QStringList()).toStringList().toSet();
}

void PatchManagerObject::setAppliedPatches(const QSet<QString> &patches)
{
    putSettings(QStringLiteral("applied"), QStringList(patches.toList()));
}

void PatchManagerObject::getVersion()
{
    qDebug() << Q_FUNC_INFO;
    m_osRelease = QSettings("/etc/os-release", QSettings::IniFormat).value("VERSION_ID").toString();
    qDebug() << "Received OS version:" << m_osRelease;
    lateInitialize();
}

void PatchManagerObject::lateInitialize()
{
    qDebug() << Q_FUNC_INFO;

    QFile file (AUSMT_INSTALLED_LIST_FILE);
    if (file.exists()) {
        qWarning() << Q_FUNC_INFO << "Found existing ausmt package list, importing applied patches list";
        if (file.open(QFile::ReadOnly)) {
            while (!file.atEnd()) {
                const QString line = QString::fromLatin1(file.readLine());
                const QStringList splitted = line.split(QChar(' '));
                if (splitted.count() == 2) {
                    m_appliedPatches.insert(splitted.first());
                    qDebug() << Q_FUNC_INFO << splitted.first();
                }
            }
            file.close();
        }
        qWarning() << Q_FUNC_INFO << "Removing ausmt package list" <<
        file.remove();
        setAppliedPatches(m_appliedPatches);
    }

    bool needClear = false;
    QDir ausmtBackup(AUSMT_BACKUP_DIR);
    QDir oldpm3cache(QStringLiteral("/var/lib/patchmanager3/patches"));
    if (ausmtBackup.exists()) {
        qWarning() << Q_FUNC_INFO << "Found ausmt backup directory, preforming fakeroot clear";

        ausmtBackup.removeRecursively();
        needClear = true;
    }

    if (oldpm3cache.exists()) {
        qWarning() << Q_FUNC_INFO << "Found old backup directory, preforming fakeroot clear";

        oldpm3cache.removeRecursively();
        needClear = true;
    }

    if (needClear) {
        clearFakeroot();
    }
    refreshPatchList();

    QDir cache(PATCHES_ADDITIONAL_DIR);
    if ((cache.exists() && cache.entryList(QDir::NoDotAndDotDot | QDir::Dirs).count() > 0)
            || getSettings(QStringLiteral("applyOnBoot"), false).toBool()) {
        prepareCacheRoot();
        startLocalServer();
    }

    INotifyWatcher *mainWatcher = new INotifyWatcher(this);
    mainWatcher->addPaths({ PATCHES_DIR });
    connect(mainWatcher, &INotifyWatcher::contentChanged, [this](const QString &path, bool created) {
        qDebug() << Q_FUNC_INFO << "contentChanged:" << path << "created:" << created;
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

    registerDBus();
    checkForUpdates();
}

QList<QVariantMap> PatchManagerObject::listPatchesFromDir(const QString &dir, QSet<QString> &existingPatches, bool existing)
{
    QList<QVariantMap> patches;
    QDir root (dir);
    for (const QString &patchPath : root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        if (existingPatches.contains(patchPath)) {
            continue;
        }
        QVariantMap patch;
        bool ok = makePatch(root, patchPath, patch, existing);
        if (!ok) {
            continue;
        }
        patches.append(patch);
        existingPatches.insert(patchPath);
    }
    return patches;
}

PatchManagerObject::PatchManagerObject(QObject *parent)
    : QObject(parent)
    , m_sbus(s_sessionBusConnection)
{
}

PatchManagerObject::~PatchManagerObject()
{
    if (m_dbusRegistered) {
        QDBusConnection connection = QDBusConnection::systemBus();
        connection.unregisterService(DBUS_SERVICE_NAME);
        connection.unregisterObject(DBUS_PATH_NAME);
    }
}

void PatchManagerObject::registerDBus()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doRegisterDBus), Qt::QueuedConnection);
}

void PatchManagerObject::waitForLipstick()
{
    qDebug() << Q_FUNC_INFO;
}

void PatchManagerObject::startLocalServer()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doStartLocalServer), Qt::QueuedConnection);
}

void PatchManagerObject::doRegisterDBus()
{
    qDebug() << Q_FUNC_INFO;
    if (m_dbusRegistered) {
        return;
    }

    QDBusConnection connection = QDBusConnection::systemBus();

    if (connection.interface()->isServiceRegistered(DBUS_SERVICE_NAME)) {
        qWarning() << Q_FUNC_INFO << "Service already registered:" << DBUS_SERVICE_NAME;
        return;
    }

    if (!connection.registerObject(DBUS_PATH_NAME, this)) {
        qCritical() << Q_FUNC_INFO << "Cannot register object:" << DBUS_PATH_NAME;
        QCoreApplication::quit();
        return;
    }

    qWarning() << Q_FUNC_INFO << "Object registered:" << DBUS_PATH_NAME;

    if (!connection.registerService(DBUS_SERVICE_NAME)) {
        qCritical() << Q_FUNC_INFO << "Cannot register D-Bus service:" << DBUS_SERVICE_NAME;
        QCoreApplication::quit();
        return;
    }

    m_adaptor = new PatchManagerAdaptor(this);
    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        m_adaptor->installEventFilter(this);
    }
    qWarning() << Q_FUNC_INFO << "Service registered:" << DBUS_SERVICE_NAME;
    m_dbusRegistered = true;
}

void PatchManagerObject::doPrepareCacheRoot()
{
    qWarning() << Q_FUNC_INFO;
    // TODO: think about security issues here


    QStringList order = getSettings(QStringLiteral("order"), QStringList()).toStringList();

    bool success = true;

    if (m_adaptor) {
        emit m_adaptor->autoApplyingStarted(m_appliedPatches.count());
    }

    for (const QString &patchName : order) {
        if (m_appliedPatches.contains(patchName)) {
            if (m_adaptor) {
                emit m_adaptor->autoApplyingPatch(getPatchName(patchName));
            }
            if (!doPatch(patchName, true)) {
                if (m_adaptor) {
                    emit m_adaptor->autoApplyingFailed(getPatchName(patchName));
                }
                m_appliedPatches.remove(patchName);
                success = false;
            }
        }
    }

    for (const QString &patchName : QSet<QString>(m_appliedPatches)) {
        if (!order.contains(patchName)) {
            if (m_adaptor) {
                emit m_adaptor->autoApplyingPatch(getPatchName(patchName));
            }
            if (!doPatch(patchName, true)) {
                if (m_adaptor) {
                    emit m_adaptor->autoApplyingFailed(getPatchName(patchName));
                }
                m_appliedPatches.remove(patchName);
                success = false;
            }
        }
    }

    if (m_adaptor) {
        emit m_adaptor->autoApplyingFinished(success);
    }

    if (!success) {
        setAppliedPatches(m_appliedPatches);
        refreshPatchList();
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
        qDebug() << Q_FUNC_INFO << "processing:" << fileName;
        QFileInfo fi(fileName);

        QDir fakeDir(QStringLiteral("%1%2").arg(s_patchmanagerCacheRoot, fi.absoluteDir().absolutePath()));
        if (apply && !fakeDir.exists()) {
            qWarning() << Q_FUNC_INFO << "creating:" << fakeDir.absolutePath();
            QDir::root().mkpath(fakeDir.absolutePath());
        }

        if (apply && !fi.absoluteDir().exists()) {
            if (tryToLinkFakeParent(fi.absoluteDir().absolutePath())) {
                continue;
            }
        }

        if (apply && checkIsFakeLinked(fi.absoluteDir().absolutePath())) {
            continue;
        }

        const QString fakeFileName = QStringLiteral("%1/%2").arg(fakeDir.absolutePath(), fi.fileName());

        if (apply && !fi.exists()) {
            bool link_ret = QFile::link(fakeFileName, fi.absoluteFilePath());
            qWarning() << Q_FUNC_INFO << "linking" << fileName << "to:" << fakeFileName << link_ret;
            continue;
        }

        if (!apply && fi.isSymLink()) {
            bool remove_ret = QFile::remove(fi.absoluteFilePath());
            qWarning() << Q_FUNC_INFO << "Removing symlink" << fileName << "to" << fakeFileName << remove_ret;
        }

        if (QFileInfo::exists(fakeFileName)) {
            if (apply) {
                m_originalWatcher->addPath(fileName);
                continue;
            }

            if (m_fileToPatch.value(fileName).length() > 1) { // TODO: should check only applied patches?
                continue;
            }

            m_originalWatcher->removePath(fileName);
            bool remove_ret = QFile::remove(fakeFileName);
            qWarning() << Q_FUNC_INFO << "Removing" << fakeFileName << remove_ret;
        } else {
            if (!apply) {
                tryToUnlinkFakeParent(fi.absoluteDir().absolutePath());
                continue;
            }

            struct stat fileStat;
            if (stat(fileName.toLatin1().constData(), &fileStat) < 0) {
                qWarning() << Q_FUNC_INFO << "warning!" << fileName << "could not be stat!";
                continue;
            }

            bool copy_ret = QFile::copy(fileName, fakeFileName);
            qWarning() << Q_FUNC_INFO << "Copying" << fileName << "to:" << fakeFileName << copy_ret;
            m_originalWatcher->addPath(fileName);

            chmod(fakeFileName.toLatin1().constData(), fileStat.st_mode);
            int chown_ret = chown(fakeFileName.toLatin1().constData(), fileStat.st_uid, fileStat.st_gid);
            if (chown_ret) {
                perror(fakeFileName.toLatin1().constData());
            }
        }
    }
}

void PatchManagerObject::doStartLocalServer()
{
    qDebug() << Q_FUNC_INFO;

    if (!getLoaded()) {
        m_serverThread->start();
        if (m_adaptor) {
            emit m_adaptor->loadedChanged(true);
        }
    }
}

void PatchManagerObject::initialize()
{
    qDebug() << Q_FUNC_INFO << "Patchmanager:" << qApp->applicationVersion();

    QTranslator *translator = new QTranslator(this);
    bool success = translator->load(QLocale(getLang()),
                                   QStringLiteral("settings-patchmanager"),
                                   QStringLiteral("-"),
                                   QStringLiteral("/usr/share/translations/"),
                                   QStringLiteral(".qm"));
    qDebug() << Q_FUNC_INFO << "Translator loaded:" << success;

    success = qApp->installTranslator(translator);
    qDebug() << Q_FUNC_INFO << "Translator installed:" << success;

    m_nam = new QNetworkAccessManager(this);
    m_settings = new QSettings(s_newConfigLocation, QSettings::IniFormat, this);

    qDebug() << Q_FUNC_INFO << "Environment:";

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &key : env.keys()) {
        qDebug() << Q_FUNC_INFO << key << "=" << env.value(key);
    }

    QFile preload(QStringLiteral("/etc/ld.so.preload"));
    if (preload.exists()) {
        qDebug() << Q_FUNC_INFO << "ld.so.preload:";
        if (!preload.open(QFile::ReadOnly)) {
            qWarning() << Q_FUNC_INFO << "Can't open ld.so.preload!";
        }
        qDebug().noquote() << Q_FUNC_INFO << preload.readAll();
    } else {
        qWarning() << Q_FUNC_INFO << "ld.so.preload does not exists!";
    }

    qDebug() << Q_FUNC_INFO << PM_APPLY;
    QFileInfo pa(PM_APPLY);
    if (pa.exists()) {
        qDebug() << Q_FUNC_INFO << pa.permissions();
    } else {
        qWarning() << Q_FUNC_INFO << "Does not exists!";
    }

    qDebug() << Q_FUNC_INFO << PM_UNAPPLY;
    QFileInfo pu(PM_UNAPPLY);
    if (pu.exists()) {
        qDebug() << Q_FUNC_INFO << pu.permissions();
    } else {
        qWarning() << Q_FUNC_INFO << "Does not exists!";
    }

    if (!QFileInfo::exists(s_newConfigLocation) && QFileInfo::exists(s_oldConfigLocation)) {
        QFile::copy(s_oldConfigLocation, s_newConfigLocation);
    }

    if (Q_UNLIKELY(qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER"))) {
        installEventFilter(this);
    }

    if (Q_UNLIKELY(qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))) {
        qWarning() << Q_FUNC_INFO << "Session bus address is not set! Please check environment configuration!";
        qDebug() << Q_FUNC_INFO << "Injecting DBUS_SESSION_BUS_ADDRESS...";
        qputenv("DBUS_SESSION_BUS_ADDRESS", QByteArrayLiteral("unix:path=/run/user/100000/dbus/user_bus_socket"));
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PatchManagerObject::onTimerAction);
    m_timer->setSingleShot(false);
    m_timer->setTimerType(Qt::VeryCoarseTimer);
    m_timer->setInterval(60 * 60 * 1000); // 60 min
    m_timer->start();

    m_sessionBusConnector = new QTimer(this);
    connect(m_sessionBusConnector, &QTimer::timeout, this, [this]() {
        QDBusConnection test = QDBusConnection::connectToBus(QDBusConnection::SessionBus, s_sessionBusConnection);
        if (!test.isConnected()) {
            QDBusConnection::disconnectFromBus(s_sessionBusConnection);
        } else {
            m_sbus = test;
            m_sessionBusConnector->stop();
            qDebug() << Q_FUNC_INFO << "Connected to session bus!";

            m_sbus.connect(QString(),
                           QStringLiteral("/org/freedesktop/systemd1/unit/lipstick_2eservice"),
                           QStringLiteral("org.freedesktop.DBus.Properties"),
                           QStringLiteral("PropertiesChanged"), this, SLOT(onLipstickChanged(QString,QVariantMap,QStringList)));

            m_sbus.connect(QString(),
                           QStringLiteral("/StoreClient"),
                           QStringLiteral("com.jolla.jollastore"),
                           QStringLiteral("osUpdateProgressChanged"), this, SLOT(onOsUpdateProgress(int)));
        }
    });
    m_sessionBusConnector->setSingleShot(false);
    m_sessionBusConnector->setTimerType(Qt::VeryCoarseTimer);
    m_sessionBusConnector->setInterval(1000); // 1 second
    m_sessionBusConnector->start();

    m_originalWatcher = new QFileSystemWatcher(this);
    connect(m_originalWatcher, &QFileSystemWatcher::fileChanged, this, &PatchManagerObject::onOriginalFileChanged);

    m_localServer = new QLocalServer(nullptr); // controlled by separate thread
    m_localServer->setSocketOptions(QLocalServer::WorldAccessOption);
    m_localServer->setMaxPendingConnections(2147483647);

    m_serverThread = new QThread(this);
    connect(m_serverThread, &QThread::finished, this, [this](){
        m_localServer->close();
    });
    connect(m_localServer, &QLocalServer::newConnection, this, &PatchManagerObject::startReadingLocalServer, Qt::DirectConnection);
    connect(m_serverThread, &QThread::started, this, [this](){
        bool listening = m_localServer->listen(s_patchmanagerSocket);
        if (!listening // not listening
                && m_localServer->serverError() == QAbstractSocket::AddressInUseError // because of AddressInUseError
                && QFileInfo::exists(s_patchmanagerSocket) // socket file already exists
                && QFile::remove(s_patchmanagerSocket)) { // and successfully removed it
            qWarning() << Q_FUNC_INFO << "Removed old stuck socket";
            listening = m_localServer->listen(s_patchmanagerSocket); // try to start lisening again
        }
        qDebug() << Q_FUNC_INFO << "Server listening:" << listening;
        if (!listening) {
            qWarning() << Q_FUNC_INFO << "Server error:" << m_localServer->serverError() << m_localServer->errorString();
        }
    }, Qt::DirectConnection);
    m_localServer->moveToThread(m_serverThread);

    m_journal = new Journal(this);
    connect(m_journal, &Journal::matchFound, this, &PatchManagerObject::onFailureOccured);
    m_journal->init();

    static bool rpmConfigRead = false;
    if (!rpmConfigRead) {
        rpmReadConfigFiles(NULL, NULL);
        rpmConfigRead = true;
    }

    getVersion();
}

QString PatchManagerObject::getPatchName(const QString patch) const
{
    if (!m_metadata.contains(patch)) {
        return patch;
    }

    const QVariantMap patchData = m_metadata[patch];
    return (patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY]).toString();
}

void PatchManagerObject::restartLipstick()
{
    qDebug() << Q_FUNC_INFO;

    QMetaObject::invokeMethod(this, NAME(doRestartLipstick), Qt::QueuedConnection);
}

void PatchManagerObject::doRestartLipstick()
{
    qDebug() << Q_FUNC_INFO;

    restartService(QStringLiteral("lipstick.service"));
}

void PatchManagerObject::restartKeyboard()
{
    qDebug() << Q_FUNC_INFO;

    QMetaObject::invokeMethod(this, NAME(doRestartKeyboard), Qt::QueuedConnection);
}

void PatchManagerObject::doRestartKeyboard()
{
    qDebug() << Q_FUNC_INFO;

    restartService(QStringLiteral("maliit-server.service"));
}

void PatchManagerObject::restartService(const QString &serviceName)
{
    qDebug() << Q_FUNC_INFO << serviceName;

    QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                                    QStringLiteral("/org/freedesktop/systemd1"),
                                                    QStringLiteral("org.freedesktop.systemd1.Manager"),
                                                    QStringLiteral("RestartUnit"));
    m.setArguments({ serviceName, QStringLiteral("replace") });
    if (!m_sbus.send(m)) {
        qWarning() << Q_FUNC_INFO << "Error sending message";
        qWarning() << Q_FUNC_INFO << "Invoking systemctl:" <<
                    QProcess::execute(BIN_SYSTEMCTL_U, { QStringLiteral("--no-block"), QStringLiteral("restart"), serviceName });
    }
}

void PatchManagerObject::resetSystem()
{
    qDebug() << Q_FUNC_INFO;

    unapplyAllPatches();

    QStringList patchedFiles;
    QDir patchesDir(PATCHES_DIR);
    for (const QString &patchFolder : patchesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        qDebug() << Q_FUNC_INFO << "Processing patch:" << patchFolder;
        QFile patchFile(QStringLiteral("%1/%2/unified_diff.patch").arg(PATCHES_DIR, patchFolder));
        if (!patchFile.exists() || !patchFile.open(QFile::ReadOnly)) {
            continue;
        }
        while (!patchFile.atEnd()) {
            QByteArray line = patchFile.readLine();
            if (!line.startsWith(QByteArrayLiteral("+++ "))) {
                continue;
            }
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
            if (!patchedFiles.contains(path)) {
                qDebug() << Q_FUNC_INFO << "Found patched file:" << path;
                patchedFiles.append(path);
            }
        }
    }

    QStringList packages;
    for (const QString &file : patchedFiles) {
        qDebug() << Q_FUNC_INFO << "Processing file:" << file;

        QProcess rpmProc;
        rpmProc.start(BIN_RPM, { QStringLiteral("-qf"), QStringLiteral("--qf"), QStringLiteral("%{NAME}"), file });
        if (!rpmProc.waitForFinished(5000) || rpmProc.exitCode() != 0) {
            continue;
        }
        const QString package = QString::fromLatin1(rpmProc.readAllStandardOutput());
        if (package.isEmpty()) {
            continue;
        } else if (!packages.contains(package)) {
            qDebug() << Q_FUNC_INFO << "Found package to reinstall:" << package;
            packages.append(package);
        }
    }

    if (packages.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Nothing to reinstall!";
        QCoreApplication::exit(0);
        return;
    }

    qDebug() << Q_FUNC_INFO << "Refreshing repositories...";
    QProcess refreshProc;
    refreshProc.start(BIN_PKCON, { QStringLiteral("refresh") });
    refreshProc.waitForFinished(-1);

    for (const QString &package : packages) {
        qDebug() << Q_FUNC_INFO << "Reinstalling:" << package;

        QProcess pkconProc;
        pkconProc.start(BIN_PKCON, { QStringLiteral("install"), QStringLiteral("-y"), package });
        pkconProc.waitForFinished(-1);

        if (pkconProc.exitCode() == 0) {
            qDebug() << Q_FUNC_INFO << "Reinstalled!";
        } else {
            qDebug() << Q_FUNC_INFO << "Problem while reinstalling!";
        }
    }

    QCoreApplication::exit(0);
}

void PatchManagerObject::clearFakeroot()
{
    qDebug() << Q_FUNC_INFO;

    eraseRecursively(s_patchmanagerCacheRoot);
    qDebug() << Q_FUNC_INFO << "Directory" << s_patchmanagerCacheRoot << "is empty:" <<
    QDir::root().rmpath(s_patchmanagerCacheRoot);

    qDebug() << Q_FUNC_INFO << "Directory" << PATCHES_ADDITIONAL_DIR << "is empty:" <<
    QDir(PATCHES_ADDITIONAL_DIR).removeRecursively();

    qDebug() << Q_FUNC_INFO << "Making clean cache:" <<
    QDir::root().mkpath(s_patchmanagerCacheRoot);
}

QString PatchManagerObject::getRpmName(const QString &rpm) const
{
    const QString info = rpm.section('-', -2);
    const QString name = rpm.left(rpm.length() - info.length() - 1);
    return name;
}

void PatchManagerObject::process()
{
    const QStringList args = QCoreApplication::arguments();

    if (args.count() == 2 && args[1] == QStringLiteral("--daemon")) {
        initialize();
    } else if (args[1] == QStringLiteral("--reset-system")) {
         resetSystem();
         QCoreApplication::exit(2);
         return;
    } else if (args.count() > 1) {
        QDBusConnection connection = QDBusConnection::systemBus();
        qDebug() << Q_FUNC_INFO << "Have arguments, sending dbus message and quit";

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
        connection.call(msg);

        QCoreApplication::exit(0);
        return;
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

QVariantMap PatchManagerObject::applyPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    QDBusMessage msg;
    if (calledFromDBus()) {
        setDelayedReply(true);
        msg = message();
    }
    QMetaObject::invokeMethod(this, NAME(doPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{QStringLiteral("name"), patch},
                                                              {QStringLiteral("user_request"), true}})),
                              Q_ARG(QDBusMessage, msg),
                              Q_ARG(bool, true));
    return QVariantMap();
}

QVariantMap PatchManagerObject::unapplyPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    QDBusMessage msg;
    if (calledFromDBus()) {
        if (!m_appliedPatches.contains(patch)) {
            const QVariantMap reply = {{ QStringLiteral("ok"), true }, { QStringLiteral("log"), QStringLiteral("not applied") }};
            return reply;
        }

        setDelayedReply(true);
        msg = message();
    }
    QMetaObject::invokeMethod(this, NAME(doPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{QStringLiteral("name"), patch},
                                                              {QStringLiteral("user_request"), true}})),
                              Q_ARG(QDBusMessage, msg),
                              Q_ARG(bool, false));
    return QVariantMap();
}

bool PatchManagerObject::unapplyAllPatches()
{
    qDebug() << Q_FUNC_INFO;

    clearFakeroot();

    qDebug() << Q_FUNC_INFO << "Toggle restart services...";
    for (const QString &appliedPatch : m_appliedPatches) {
        patchToggleService(appliedPatch);
    }

    qDebug() << Q_FUNC_INFO << "Resetting variables...";
    m_appliedPatches.clear();
    setAppliedPatches(m_appliedPatches);

    refreshPatchList();

    return true;
}

bool PatchManagerObject::installPatch(const QString &patch, const QString &version, const QString &url)
{
    qDebug() << Q_FUNC_INFO << patch;
    if (calledFromDBus()) {
        setDelayedReply(true);
    }
    QMetaObject::invokeMethod(this, NAME(doInstallPatch), Qt::QueuedConnection,
                              Q_ARG(QVariantMap, QVariantMap({{"patch", patch}, {"version", version}, {"url", url}})),
                              Q_ARG(QDBusMessage, message()));
    return true;
}

bool PatchManagerObject::uninstallPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    if (calledFromDBus()) {
        setDelayedReply(true);
    }

    if (isPatchApplied(patch)) {
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
    if (calledFromDBus()) {
        setDelayedReply(true);
    }

    if (isPatchApplied(patch)) {
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
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(requestCheckForUpdates), Qt::QueuedConnection);
}

QVariantMap PatchManagerObject::getUpdates() const
{
    return m_updates;
}

bool PatchManagerObject::putSettings(const QString &name, const QDBusVariant &value)
{
    return putSettings(name, value.variant());
}

QDBusVariant PatchManagerObject::getSettings(const QString &name, const QDBusVariant &def)
{
    return QDBusVariant(getSettings(name, def.variant()));
}

bool PatchManagerObject::putSettings(const QString &name, const QVariant &value)
{
    qDebug() << Q_FUNC_INFO << name << value;
    QString key = QStringLiteral("settings/%1").arg(name);
    QVariant old = m_settings->value(key);
    if (old != value) {
        m_settings->setValue(key ,value);
        return true;
    }
    return false;
}

QVariant PatchManagerObject::getSettings(const QString &name, const QVariant &def) const
{
    QString key = QStringLiteral("settings/%1").arg(name);
    const QVariant value = m_settings->value(key, def);
    qDebug() << Q_FUNC_INFO << name << def << value;
    return value;
}

QString PatchManagerObject::maxVersion(const QString &version1, const QString &version2)
{
    const QStringList vnums1 = version1.split(QChar('.'));
    const QStringList vnums2 = version2.split(QChar('.'));

    if (vnums1.count() < 3 || vnums2.count() < 3) {
        return version1;
    }

    for (int i = 0; i < 3; i++) {
        const QString vnum1 = vnums1.at(i);
        const QString vnum2 = vnums2.at(i);

        bool ok = false;
        const int num1 = vnum1.toInt(&ok);
        if (!ok) {
            continue;
        }
        const int num2 = vnum2.toInt(&ok);
        if (!ok) {
            continue;
        }
        if (num1 == num2) {
            continue;
        }
        if (num1 > num2) {
            return version1;
        }
        return version2;
    }

    return version1;
}

void PatchManagerObject::restartServices()
{
    qDebug() << Q_FUNC_INFO << m_toggleServices;

    if (m_toggleServices.isEmpty()) {
        return;
    }

    for (const QString &category : m_toggleServices.keys()) {
        qDebug() << Q_FUNC_INFO << category;
        if (category == HOMESCREEN_CODE || category == SILICA_CODE) {
            restartLipstick();
        } else if (category == KEYBOARD_CODE) {
            restartKeyboard();
        } else {
            QHash<QString, QString> categoryToProcess = {
                { BROWSER_CODE, QStringLiteral("sailfish-browser") },
                { CAMERA_CODE, QStringLiteral("jolla-camera") },
                { CALENDAR_CODE, QStringLiteral("jolla-calendar") },
                { CLOCK_CODE, QStringLiteral("jolla-clock") },
                { CONTACTS_CODE, QStringLiteral("jolla-contacts") },
                { EMAIL_CODE, QStringLiteral("jolla-email") },
                { GALLERY_CODE, QStringLiteral("jolla-gallery") },
                { MEDIA_CODE, QStringLiteral("jolla-mediaplayer") },
                { MESSAGES_CODE, QStringLiteral("jolla-messages") },
                { PHONE_CODE, QStringLiteral("voicecall-ui") },
                { SETTINGS_CODE, QStringLiteral("jolla-settings") },
            };

            if (!categoryToProcess.contains(category)) {
                qWarning() << Q_FUNC_INFO << "Invalid category:" << category;
                continue;
            }

            QStringList arguments;
            arguments << categoryToProcess[category];
            QProcess::execute(QStringLiteral("killall"), arguments);
        }
    }

    m_toggleServices.clear();

    if (m_adaptor) {
        emit m_adaptor->toggleServicesChanged(false);
    }
}

void PatchManagerObject::patchToggleService(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;

    if (!m_metadata.contains(patch)) {
        return;
    }

    const QString &category = m_metadata[patch][CATEGORY_KEY].toString();

    if (m_toggleServices.contains(category) && m_toggleServices[category].contains(patch)) {
        return;
    }

    QStringList patches = m_toggleServices[category];
    patches.append(patch);
    m_toggleServices[category] = patches;

    if (m_adaptor) {
        emit m_adaptor->toggleServicesChanged(true);
    }
}

QStringList PatchManagerObject::getToggleServicesList() const
{
    return m_toggleServices.keys();
}

bool PatchManagerObject::getToggleServices() const
{
    return !m_toggleServices.isEmpty();
}

bool PatchManagerObject::getFailure() const
{
    return m_failed;
}

bool PatchManagerObject::getLoaded() const
{
    return m_serverThread->isRunning();
}

void PatchManagerObject::resolveFailure()
{
    qDebug() << Q_FUNC_INFO;

    if (!m_failed) {
        return;
    }

    if (!getLoaded()) {
        startLocalServer();
    }

    m_failed = false;
    if (m_adaptor) {
        emit m_adaptor->failureChanged(m_failed);
    }
}

void PatchManagerObject::loadRequest(bool apply)
{
    qDebug() << Q_FUNC_INFO << apply;

    if (getLoaded()) {
        qWarning() << Q_FUNC_INFO << "Server thread already working!";
        return;
    }

    if (apply) {
        prepareCacheRoot();
    } else {
        unapplyAllPatches();
    }

    startLocalServer();

    if (apply && m_appliedPatches.count() > 0) {
        restartLipstick();
    }
}

void PatchManagerObject::lipstickChanged(const QString &state)
{
    qDebug() << Q_FUNC_INFO << state;

    if (!getLoaded() && !m_failed && !getSettings(QStringLiteral("applyOnBoot"), false).toBool()) {
        qDebug() << Q_FUNC_INFO << "Calling patch applier after boot";
        QTimer::singleShot(20000, this, [this](){
            QDBusMessage showPatcher = QDBusMessage::createMethodCall(QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                                      QStringLiteral("/"),
                                                                      QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                                      QStringLiteral("show"));
            qDebug() << Q_FUNC_INFO << m_sbus.send(showPatcher);
        });
    }
}

QString PatchManagerObject::getPatchmanagerVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString PatchManagerObject::getOsVersion() const
{
    return m_osRelease;
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
        qDebug() << Q_FUNC_INFO << watched << event->type();
    }
    return QObject::eventFilter(watched, event);
}

void PatchManagerObject::onLipstickChanged(const QString &, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    qDebug() << Q_FUNC_INFO << changedProperties << invalidatedProperties;
    if (invalidatedProperties.contains(QStringLiteral("ActiveState"))) {
        return;
    }

    const QString activeState = changedProperties.value(QStringLiteral("ActiveState"), QStringLiteral("unknown")).toString();
    qDebug() << Q_FUNC_INFO << activeState;
    if (activeState == QStringLiteral("failed")) {
        qWarning() << Q_FUNC_INFO << "Detected lipstick crash, deactivating all patches";
        unapplyAllPatches();
    } else if (activeState == QStringLiteral("active") && !getLoaded() && !m_failed && !getSettings(QStringLiteral("applyOnBoot"), false).toBool()) {
        qDebug() << Q_FUNC_INFO << "Calling patch applier after boot";
        QTimer::singleShot(5000, this, [this](){
            QDBusMessage showPatcher = QDBusMessage::createMethodCall(QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                                      QStringLiteral("/"),
                                                                      QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                                      QStringLiteral("show"));
            qDebug() << Q_FUNC_INFO << m_sbus.send(showPatcher);
        });
    }
}

void PatchManagerObject::onOsUpdateProgress(int progress)
{
    if (progress <= 0) {
        return;
    }

    qWarning() << Q_FUNC_INFO << "Detected os update, disabling patches!";
    unapplyAllPatches();
}

void PatchManagerObject::onTimerAction()
{
    qDebug() << Q_FUNC_INFO;
    checkForUpdates();
}

void PatchManagerObject::startReadingLocalServer()
{
    QLocalSocket *clientConnection = m_localServer->nextPendingConnection();
    if (!clientConnection) {
        qWarning() << Q_FUNC_INFO << "Got empty connection!";
        return;
    }
    if (clientConnection->state() != QLocalSocket::ConnectedState) {
        clientConnection->waitForConnected();
    }
    connect(clientConnection, &QLocalSocket::disconnected, this, [clientConnection](){
        clientConnection->deleteLater();
    }, Qt::DirectConnection);
    connect(clientConnection, &QLocalSocket::readyRead, this, [this, clientConnection](){
        if (!clientConnection) {
            qWarning() << Q_FUNC_INFO << "Can not get socket!";
            return;
        }
        const qint64 bytes = clientConnection->bytesAvailable();
        if (bytes <= 0) {
            clientConnection->disconnectFromServer();
            return;
        }
        const QByteArray request = clientConnection->readAll();
        QByteArray payload;
        const QString fakePath = QStringLiteral("%1%2").arg(s_patchmanagerCacheRoot, QString::fromLatin1(request));
        if (!m_failed && QFileInfo::exists(fakePath)) {
            payload = fakePath.toLatin1();
            if (qEnvironmentVariableIsSet("PM_DEBUG_SOCKET")) {
                qWarning() << Q_FUNC_INFO << "Requested:" << request << "Sending:" << payload;
            }
        } else {
            payload = request;
            if (qEnvironmentVariableIsSet("PM_DEBUG_SOCKET")) {
                qWarning() << Q_FUNC_INFO << "Requested:" << request << "Not changing";
            }
        }
        clientConnection->write(payload);
        clientConnection->flush();
//        clientConnection->waitForBytesWritten();
    }, Qt::DirectConnection);
}

void PatchManagerObject::onOriginalFileChanged(const QString &path)
{
    qWarning() << Q_FUNC_INFO << path;

    if (m_failed || !getLoaded()) {
        return;
    }

    if (!m_fileToPatch.contains(path)) {
        return;
    }

    QStringList patches = m_fileToPatch[path];
    for (const QString &patch : QStringList(patches)) {
        if (!isPatchApplied(patch)) {
            patches.removeAll(patch);
        }
    }

    qDebug() << Q_FUNC_INFO << patches;

    QStringList order = getSettings(QStringLiteral("order"), QStringList()).toStringList();
    std::reverse(std::begin(order), std::end(order));

    bool success = true;

    for (const QString &patch : order) {
        if (patches.contains(patch)) {
            success &= doPatch(patch, false);
        }
    }

    for (const QString &patch : patches) {
        if (!order.contains(patch)) {
            success &= doPatch(patch, false);
        }
    }

    std::reverse(std::begin(order), std::end(order));

    for (const QString &patch : order) {
        if (patches.contains(patch)) {
            success &= doPatch(patch, true);
        }
    }

    for (const QString &patch : patches) {
        if (!order.contains(patch)) {
            success &= doPatch(patch, true);
        }
    }

    if (!success) {
        clearFakeroot();
        doPrepareCacheRoot();
    }
}

void PatchManagerObject::onFailureOccured()
{
    qDebug() << Q_FUNC_INFO;

    if (getLoaded()) {
        m_serverThread->quit();
    }

    if (m_failed) {
        return;
    }

    m_failed = true;
    if (m_adaptor) {
        emit m_adaptor->failureChanged(m_failed);
    }

    putSettings(QStringLiteral("applyOnBoot"), false);

    unapplyAllPatches();
    restartLipstick();
}

void PatchManagerObject::doRefreshPatchList()
{
    qDebug() << Q_FUNC_INFO;

    // load applied patches

    m_appliedPatches = getAppliedPatches();

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
            const QByteArray line = patchFile.readLine();
            if (line.startsWith(QByteArrayLiteral("+++ "))) {
                const QString toPatch = QString::fromLatin1(line.split(' ')[1].split('\t')[0].split('\n')[0]);
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
                if (!filesConflicts[path].contains(patchFolder)) {
                    filesConflicts[path].append(patchFolder);
                }

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
    qDebug() << Q_FUNC_INFO << "patchFiles:" << m_patchFiles.keys();
    qDebug() << Q_FUNC_INFO << "fileToPatch:" << m_fileToPatch.keys();

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
    qDebug() << Q_FUNC_INFO << "patchConflicts:" << patchConflicts.keys();

    // get patches

    QSet<QString> existingPatches;
    QList<QVariantMap> patches = listPatchesFromDir(PATCHES_DIR, existingPatches);
    patches.append(listPatchesFromDir(PATCHES_ADDITIONAL_DIR, existingPatches, false));
    qDebug() << Q_FUNC_INFO << "patches:" << patches.count();
//    std::sort(patches.begin(), patches.end(), patchSort);

    // fill patch conflicts and rpm names

    m_metadata.clear();
    for (QVariantMap &patch : patches) {
        const QString patchName = patch[PATCH_KEY].toString();
        if (patchConflicts.contains(patchName)) {
            patch[CONFLICTS_KEY] = patchConflicts[patchName];
        }

        //
        rpmts transactionSet = rpmtsCreate();

        char filename[255] = {0};
        sprintf(filename, "/usr/share/patchmanager/patches/%s/unified_diff.patch", patchName.toLatin1().constData());

        rpmdbMatchIterator it = rpmtsInitIterator(transactionSet, RPMDBI_BASENAMES, filename, 0);

        if (Header header = rpmdbNextIterator(it)) {
            rpmtd tagData = rpmtdNew();

            QString name;
            QString version;
            QString release;
            QString arch;

            if (headerGet(header, RPMTAG_NAME, tagData, HEADERGET_DEFAULT)) {
                const char *value = rpmtdGetString(tagData);
                if (value) {
                    name = QString::fromLatin1(value);
                }
            }

            if (headerGet(header, RPMTAG_VERSION, tagData, HEADERGET_DEFAULT)) {
                const char *value = rpmtdGetString(tagData);
                if (value) {
                    version = QString::fromLatin1(value);
                }
            }

            if (headerGet(header, RPMTAG_RELEASE, tagData, HEADERGET_DEFAULT)) {
                const char *value = rpmtdGetString(tagData);
                if (value) {
                    release = QString::fromLatin1(value);
                }
            }

            if (headerGet(header, RPMTAG_ARCH, tagData, HEADERGET_DEFAULT)) {
                const char *value = rpmtdGetString(tagData);
                if (value) {
                    arch = QString::fromLatin1(value);
                }
            }

            rpmtdFree(tagData);

            patch[RPM_KEY] = QStringLiteral("%1-%2-%3.%4").arg(name, version, release, arch);
        }

        rpmdbFreeIterator(it);
        rpmtsFree(transactionSet);
        //

        m_metadata[patchName] = patch;
    }

    qDebug() << Q_FUNC_INFO << "metadata:" << m_metadata.keys();

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
    QStringList order = getSettings(QStringLiteral("order"), QStringList()).toStringList();
    qDebug() << Q_FUNC_INFO << "order:" << order;

    for (const QString &patchName : order) {
        if (m_metadata.contains(patchName)) {
            result.append(m_metadata.value(patchName));
        }
    }

    for (const QString &patchName : m_metadata.keys()) {
        if (!order.contains(patchName)) {
            result.append(m_metadata.value(patchName));
        }
    }

//    for (const QVariantMap &patch : m_metadata) {

//        result.append(patch);
//    }
    sendMessageReply(message, result);
}

bool PatchManagerObject::doPatch(const QString &patchName, bool apply, QString *patchLog)
{
    qDebug() << Q_FUNC_INFO << patchName << apply;

    if (apply) {
        doPrepareCache(patchName, apply);
    }

    QProcess process;
    process.setProgram(apply ? PM_APPLY : PM_UNAPPLY);

    QStringList arguments;
    arguments.append(patchName);

    process.setArguments(arguments);
    qDebug() << Q_FUNC_INFO << "Starting:" << process.program() << process.arguments();
    process.start();
    process.waitForFinished(-1);
    const bool ret = process.exitCode() == 0;
    qDebug() << Q_FUNC_INFO << "Success:" << ret;
    const QString log = QString::fromUtf8(process.readAllStandardOutput());
    qDebug().noquote() << Q_FUNC_INFO << log;
    if (patchLog) {
        *patchLog = log;
    }

    if ((!apply && ret) || (apply && !ret)) {
        doPrepareCache(patchName, false);
    }

    return ret;
}

void PatchManagerObject::doPatch(const QVariantMap &params, const QDBusMessage &message, bool apply)
{
    qDebug() << Q_FUNC_INFO << params << apply;
    const QString &patch = params.value(QStringLiteral("name")).toString();
    const bool user_request = params.value(QStringLiteral("user_request"), false).toBool();
    const bool at_init = params.value(QStringLiteral("at_init"), false).toBool();

    QVariantMap patchData = m_metadata[patch];
    QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

    QString log;
    bool ok = doPatch(patch, apply, &log);
    qDebug() << Q_FUNC_INFO << "ok:" << ok;
    if (ok) {
        if (apply) {
            m_appliedPatches.insert(patch);
            const QString rpmPatch = m_metadata[patch][RPM_KEY].toString();
            if (rpmPatch.isEmpty() && user_request) {
                sendActivation(patch, m_metadata[patch][VERSION_KEY].toString());
            }
        } else {
            m_appliedPatches.remove(patch);
        }
        setAppliedPatches(m_appliedPatches);
        refreshPatchList();
        if (!at_init) {
            patchToggleService(patch);
        }
    }

    if (!params.value(QStringLiteral("dont_notify"), false).toBool()) {
        notify(displayName.toString(), apply ? ok ? NotifyActionSuccessApply : NotifyActionFailedApply : ok ? NotifyActionSuccessUnapply : NotifyActionFailedUnapply);
    }

    if (message.isDelayedReply()) {
        QVariantMap reply = {{ QStringLiteral("ok"), ok }, { QStringLiteral("log"), log }};
        qWarning() << Q_FUNC_INFO << "Sending reply";
        sendMessageReply(message, reply);
    } else {
        qWarning() << Q_FUNC_INFO << "Message is not a delayed";
    }
}

void PatchManagerObject::doResetPatchState(const QString &patch, const QDBusMessage &message)
{
    bool success = m_appliedPatches.remove(patch);
    if (success) {
        setAppliedPatches(m_appliedPatches);
    }
    sendMessageReply(message, success);
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
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            sendMessageError(message, reply->errorString());
            return;
        }
        if (reply->bytesAvailable() <= 0) {
            qDebug() << Q_FUNC_INFO << "Cannot get json";
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
        qWarning() << Q_FUNC_INFO << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << Q_FUNC_INFO << "ignoring ssl error:" << sslError.errorString();
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
    const QString &archive = QStringLiteral("%1/%2").arg(PATCHES_WORK_DIR, url.section(QChar('/'), -1));
    const QString &version = params.value(QStringLiteral("version")).toString();

    qDebug() << Q_FUNC_INFO << "Saving archive to" << archive;
    QDir workDir(PATCHES_WORK_DIR);
    if (!workDir.mkpath(PATCHES_WORK_DIR)) {
            qDebug() << Q_FUNC_INFO << QStringLiteral("Error: could not create ") << workDir;
            return;
    };
    QFile *archiveFile = new QFile(archive, this);
    if (!archiveFile->open(QFile::WriteOnly)) {
        return;
    }

    QUrl webUrl(QString(MEDIA_URL"/%1").arg(url));
    QNetworkRequest request(webUrl);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply, message, patch, archiveFile, archive, json, version, this](){
        reply->deleteLater();
        archiveFile->deleteLater();
        qDebug() << Q_FUNC_INFO << "Flushing to disk:" << archiveFile->flush();
        archiveFile->close();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            sendMessageError(message, reply->errorString());
            return;
        }
        if (!archiveFile) {
            sendMessageError(message, QStringLiteral("Lost in the wild"));
            return;
        }

        const QString patchPath = QStringLiteral("%1/%2").arg(PATCHES_DIR, patch);
        const QString jsonPath = QStringLiteral("%1/%2").arg(patchPath, PATCH_FILE);

        QDir patchDir(patchPath);
        if (patchDir.exists()) {
            patchDir.removeRecursively();
        }
        if (!archiveFile->exists() || !patchDir.mkpath(patchPath)) {
            sendMessageError(message, QStringLiteral("Error operating with files"));
            return;
        }
        QFile jsonFile(jsonPath);
        if (!jsonFile.open(QFile::WriteOnly)) {
            sendMessageError(message, QStringLiteral("Error saving json"));
            return;
        }

        jsonFile.write(json.toLatin1());
        jsonFile.close();

        QProcess proc;
        int ret = 0;
        if (archive.endsWith(QStringLiteral(".zip"))) {
            ret = proc.execute(BIN_UNZIP, {QStringLiteral("-o"), archive, QStringLiteral("-d"), patchPath });
        } else {
            QString uncompressOpt;
            if (archive.endsWith(QStringLiteral("gz"))) { uncompressOpt = QStringLiteral("-z"); }
            else if (archive.endsWith(QStringLiteral("bz2"))) { uncompressOpt = QStringLiteral("-j"); }
             // careful: GNU tar has J for everything xz/lz*, and a for automatic, BusyBox has J for .xz and a for .lzma
            else if (archive.endsWith(QStringLiteral("xz"))) { uncompressOpt = QStringLiteral("-J"); }
            else {
                qWarning() << Q_FUNC_INFO << QStringLiteral("Archive format unsupported");
                uncompressOpt = QStringLiteral("");
            }
            ret = proc.execute(BIN_TAR, {QStringLiteral("x"), uncompressOpt, QStringLiteral("-f"), archive, QStringLiteral("-C"), patchPath});
        }
        if (ret == 0) {
            if (m_updates.contains(patch)) {
                const QString upVersion = m_updates.value(patch).toString();
                const QString lastVersion = maxVersion(upVersion, version);

                if (upVersion == version || lastVersion == version) {
                    m_updates.remove(patch);
                    if (m_adaptor) {
                        emit m_adaptor->updatesAvailable(m_updates);
                    }
                }
            }
        } else {
            QString retMsg = QStringLiteral("process exited with code (%1)").arg(ret);
            if ( ret == -2 ) {
                retMsg = QStringLiteral("Process could not start (%1)").arg(ret);
            } else if ( ret == -1 ) {
                retMsg = QStringLiteral("Process crashed (%1)").arg(ret);
            }
            qDebug() << Q_FUNC_INFO << QStringLiteral("Error: extraction failed:") << proc.error() << retMsg;
            patchDir.removeRecursively();
        }

        sendMessageReply(message, ret == 0);

        if (archiveFile->exists()) {
            archiveFile->remove();
        }
    });
    QObject::connect(reply, &QNetworkReply::readyRead, [archiveFile, reply](){
        if (!archiveFile || !archiveFile->isOpen()) {
            qWarning() << Q_FUNC_INFO << "Problem with archive";
            return;
        }
        const qint64 ba = reply->bytesAvailable();
        qDebug() << Q_FUNC_INFO << "Writing" << ba << "bytes fo file:" <<
        archiveFile->write(reply->read(ba));
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError networkError){
        qWarning() << Q_FUNC_INFO << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << Q_FUNC_INFO << "ignoring ssl error:" << sslError.errorString();
        }
        reply->ignoreSslErrors(errors);
    });
}

void PatchManagerObject::doUninstallPatch(const QString &patch, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << patch;

    bool removeSuccess = false;
    const QString rpmPatch = m_metadata[patch][RPM_KEY].toString();
    if (rpmPatch.isEmpty()) {
        QDir patchDir(QStringLiteral("%1/%2").arg(PATCHES_DIR, patch));
        qDebug() << Q_FUNC_INFO << "Removing patch files" << patchDir.absolutePath();
        if (patchDir.exists()) {
            removeSuccess = patchDir.removeRecursively();
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Removing patch package" << rpmPatch;

        const int ret = QProcess::execute(BIN_PKCON, {QStringLiteral("remove"), QStringLiteral("-y"), getRpmName(rpmPatch)});
        removeSuccess = ret == 0;
    }

    qDebug() << Q_FUNC_INFO << "Success:" << removeSuccess;

//    if (removeSuccess) {
//        // TODO: gracefully update models
//        refreshPatchList();
//    }
    sendMessageReply(message, removeSuccess);
}

int PatchManagerObject::getVote(const QString &patch)
{
    return m_settings->value(QStringLiteral("votes/%1").arg(patch), 0).toInt();
}

void PatchManagerObject::doCheckVote(const QString &patch, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << patch;

    sendMessageReply(message, getVote(patch));
}

void PatchManagerObject::sendVote(const QString &patch, int action)
{
    qDebug() << Q_FUNC_INFO << patch << action;

    if (getVote(patch) == action) {
        return;
    }

    QUrl url(QStringLiteral(CATALOG_URL "/" PROJECT_PATH));
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
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
        }
    });

    QString key = QString("votes/%1").arg(patch);
    m_settings->setValue(key, action);
    m_settings->sync();
}

void PatchManagerObject::doCheckEaster(const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO;

    QUrl url(CATALOG_URL"/easter");
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, message](){
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            sendMessageError(message, reply->errorString());
            return;
        }
        if (!reply->bytesAvailable()) {
            qWarning() << Q_FUNC_INFO << "Received empty reply!";
            sendMessageError(message, QStringLiteral("Received empty reply!"));
            return;
        }

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
    qDebug() << Q_FUNC_INFO << patch << version;

    QUrl url(QStringLiteral(CATALOG_URL "/" PROJECT_PATH));
    QUrlQuery query;
    query.addQueryItem("name", patch);
    query.addQueryItem("version", version);
    query.addQueryItem("action", "activation");
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = m_nam->get(request);
    // TODO return current count of activations (or emit)
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
        }
    });
}

void PatchManagerObject::downloadPatch(const QString &patch, const QUrl &url, const QString &file)
{
    QFile *f = new QFile(file, this);
    if (!f->open(QFile::WriteOnly)) {
        return;
    }
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [patch, f, reply](){
        reply->deleteLater();
        f->deleteLater();
        if (!f) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            return;
        }
        // TODO emit download complete
    });
    QObject::connect(reply, &QNetworkReply::readyRead, [f, reply](){
        if (!f || !f->isOpen()) {
            return;
        }
        f->write(reply->read(reply->bytesAvailable()));
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError networkError){
        qWarning() << Q_FUNC_INFO << "Download file error:" << networkError << reply->errorString();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors){
        for (const QSslError &sslError : errors) {
            qWarning() << Q_FUNC_INFO << "ignoring ssl error:" << sslError.errorString();
        }
        reply->ignoreSslErrors(errors);
    });
}

void PatchManagerObject::requestDownloadCatalog(const QVariantMap &params, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << params;
    QUrl url(QStringLiteral(CATALOG_URL "/" PROJECTS_PATH));
    QUrlQuery query;
    for (const QString &key : params.keys()) {
        query.addQueryItem(key, params.value(key).toString());
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, message, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            sendMessageError(message, reply->errorString());
            return;
        }
        if (!reply->bytesAvailable()) {
            qWarning() << Q_FUNC_INFO << "Received empty reply!";
            sendMessageError(message, QStringLiteral("Received empty reply!"));
            return;
        }
        const QByteArray json = reply->readAll();

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(json, &error);

        if (error.error == QJsonParseError::NoError) {
            sendMessageReply(message, document.toVariant());
        } else {
            sendMessageError(message, error.errorString());
        }
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, message, reply](QNetworkReply::NetworkError) {
        sendMessageError(message, reply->errorString());
    });
}

void PatchManagerObject::requestDownloadPatchInfo(const QString &name, const QDBusMessage &message)
{
    qDebug() << Q_FUNC_INFO << name;
    QUrl url(QStringLiteral(CATALOG_URL "/" PROJECT_PATH));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), name);
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, message, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            sendMessageError(message, reply->errorString());
            return;
        }
        if (!reply->bytesAvailable()) {
            qWarning() << Q_FUNC_INFO << "Received empty reply!";
            sendMessageError(message, QStringLiteral("Received empty reply!"));
            return;
        }
        const QByteArray json = reply->readAll();

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(json, &error);

        if (error.error == QJsonParseError::NoError) {
            sendMessageReply(message, document.toVariant());
        } else {
            sendMessageError(message, error.errorString());
        }
    });
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, message, reply](QNetworkReply::NetworkError) {
        sendMessageError(message, reply->errorString());
    });
}

void PatchManagerObject::requestCheckForUpdates()
{
    qDebug() << Q_FUNC_INFO;

    QUrl url(QStringLiteral(CATALOG_URL "/" PROJECTS_PATH));
    QUrlQuery query;
    query.addQueryItem("version", m_osRelease);
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply *reply = m_nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << Q_FUNC_INFO << "Error:" << reply->error();
            return;
        }
        if (!reply->bytesAvailable()) {
            qWarning() << Q_FUNC_INFO << "Received empty reply!";
            return;
        }
        const QByteArray json = reply->readAll();

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(json, &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << Q_FUNC_INFO << "Error parsing json reply";
            return;
        }

        const QVariantList projects = document.toVariant().toList();
        qDebug() << Q_FUNC_INFO << "projects count:" << projects.count();
        for (const QVariant &projectVar : projects) {
            const QVariantMap project = projectVar.toMap();
            const QString projectName = project.value("name").toString();
            qDebug() << Q_FUNC_INFO << "processing:" << projectName;
            if (!m_metadata.contains(projectName)) {
                qDebug() << Q_FUNC_INFO << projectName << "patch is not installed";
                continue;
            }
            if (!m_metadata.value(projectName).value("rpm").toString().isEmpty()) {
                qDebug() << Q_FUNC_INFO << projectName << "patch installed from rpm";
                continue;
            }

            const QString patchVersion = m_metadata.value(projectName).value("version").toString();
            qDebug() << Q_FUNC_INFO << "installed:" << projectName << "version:" << patchVersion;

            QUrl purl(QStringLiteral(CATALOG_URL "/" PROJECT_PATH));
            QUrlQuery pquery;
            pquery.addQueryItem(QStringLiteral("name"), projectName);
            purl.setQuery(pquery);
            QNetworkRequest prequest(purl);
            QNetworkReply *preply = m_nam->get(prequest);
            QObject::connect(preply, &QNetworkReply::finished, [this, preply, projectName, patchVersion]() {
                preply->deleteLater();
                if (preply->error() != QNetworkReply::NoError) {
                    qDebug() << Q_FUNC_INFO << "Error:" << preply->error();
                    return;
                }
                if (!preply->bytesAvailable()) {
                    qWarning() << Q_FUNC_INFO << "Received empty reply!";
                    return;
                }
                const QByteArray json = preply->readAll();

                QJsonParseError error;
                const QJsonDocument document = QJsonDocument::fromJson(json, &error);

                if (error.error != QJsonParseError::NoError) {
                    qWarning() << Q_FUNC_INFO << projectName << "Error parsing json reply";
                    return;
                }

                const QVariantMap project = document.toVariant().toMap();

                QString latestVersion = patchVersion;

                const QVariantList files = project.value("files").toList();
                for (const QVariant &fileVar : files) {
                    const QVariantMap file = fileVar.toMap();
                    const QStringList compatible = file.value("compatible").toStringList();
                    if (!compatible.contains(m_osRelease)) {
                        continue;
                    }
                    const QString version = file.value("version").toString();
                    latestVersion = PatchManagerObject::maxVersion(latestVersion, version);
                }

                if (latestVersion == patchVersion) {
                    qDebug() << Q_FUNC_INFO << projectName << "versions match";
                    return;
                }
                qDebug() << Q_FUNC_INFO << "available:" << projectName << "version:" << latestVersion;

                if (!m_updates.contains(projectName) || m_updates.value(projectName) != latestVersion) {
                    notify(projectName, NotifyActionUpdateAvailable);

                    m_updates[projectName] = latestVersion;
                    if (m_adaptor) {
                        emit m_adaptor->updatesAvailable(m_updates);
                    }
                }
            });
        }
    });
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

void PatchManagerObject::eraseRecursively(const QString &path)
{
    qDebug() << Q_FUNC_INFO << path;

    QDir cacheDir(path);
    for (const QFileInfo &info : cacheDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsLast)) {
        if (info.isDir() && !info.isSymLink()) {
            eraseRecursively(info.absoluteFilePath());
            qDebug() << Q_FUNC_INFO << "Directory" << info.absoluteFilePath() << "is empty:" <<
            QDir::root().rmpath(info.absoluteFilePath());
        } else if (info.isFile() || info.isSymLink()) {
            QFile::remove(info.absoluteFilePath());
        }
    }

}

bool PatchManagerObject::checkIsFakeLinked(const QString &path)
{
    qDebug() << Q_FUNC_INFO << path;
    const QStringList parts = path.split(QDir::separator(), QString::SkipEmptyParts);
    QDir trial = QDir::root();
    for (const QString &part : parts) {
        if (trial.cd(part)) {
            const QFileInfo fi(trial.absolutePath());
            if (fi.isSymLink() && fi.symLinkTarget().startsWith(s_patchmanagerCacheRoot)) {
                qDebug() << Q_FUNC_INFO << path << "already have fake link:" << trial.absolutePath();
                return true;
            }
            continue;
        }
    }
    return false;
}

bool PatchManagerObject::tryToLinkFakeParent(const QString &path)
{
    qDebug() << Q_FUNC_INFO << path;
    const QStringList parts = path.split(QDir::separator(), QString::SkipEmptyParts);
    QDir trial = QDir::root();
    for (const QString &part : parts) {
        if (trial.cd(part)) {
            const QFileInfo fi(trial.absolutePath());
            if (fi.isSymLink() && fi.symLinkTarget().startsWith(s_patchmanagerCacheRoot)) {
                qDebug() << Q_FUNC_INFO << path << "already have fake link:" << trial.absolutePath();
                return true;
            }
            continue;
        }
        const QString realPath = QStringLiteral("%1/%2").arg(trial.absolutePath(), part);
        const QString fakePath = QStringLiteral("%1%2").arg(s_patchmanagerCacheRoot, realPath);
        bool link_ret = QFile::link(fakePath, realPath);
        qDebug() << Q_FUNC_INFO << "linking" << realPath << "to:" << fakePath << link_ret;
        return true;
    }
    return false;
}

bool PatchManagerObject::tryToUnlinkFakeParent(const QString &path)
{
    qDebug() << Q_FUNC_INFO << path;
    const QStringList parts = path.split(QDir::separator(), QString::SkipEmptyParts);
    QDir trial = QDir::root();
    for (const QString &part : parts) {
        if (!trial.cd(part)) {
            qWarning() << Q_FUNC_INFO << "error while trying to cd from" << trial.absolutePath() << "to:" << part;
            return false;
        }
        const QFileInfo fi(trial.absolutePath());
        if (fi.isSymLink() && fi.symLinkTarget().startsWith(s_patchmanagerCacheRoot)) {
            bool remove_ret = QFile::remove(trial.absolutePath());
            qDebug() << Q_FUNC_INFO << "removing:" << trial.absolutePath() << remove_ret;
            return true;
        }
    }
    return false;
}
