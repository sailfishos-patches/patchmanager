/*
 * Copyright (C) 2014-2016 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016-2017 Andrey Kozhevnikov <coderusinbox@gmail.com>
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

#define DBUS_GUARD(x) \
if (!calledFromDBus()) {\
    qWarning() << Q_FUNC_INFO << "This function should be only called from D-Bus!";\
    return x;\
}

static QVector<QEvent::Type> s_customEventTypes(PatchManagerEvent::PatchManagerEventTypeCount, QEvent::None);

static const QString PATCHES_DIR = QStringLiteral("/usr/share/patchmanager/patches");
static const QString PATCHES_ADDITIONAL_DIR = QStringLiteral("/var/lib/patchmanager/ausmt/patches");
static const QString PATCH_FILE = QStringLiteral("patch.json");

static const QString NAME_KEY = QStringLiteral("name");
static const QString DESCRIPTION_KEY = QStringLiteral("description");
static const QString CATEGORY_KEY = QStringLiteral("category");
static const QString INFOS_KEY = QStringLiteral("infos");
static const QString PATCH_KEY = QStringLiteral("patch");
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
    json[AVAILABLE_KEY] = available;
    json[CATEGORY_KEY] = json[CATEGORY_KEY];
    json[SECTION_KEY] = QStringLiteral("Other");
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
    , m_settings(new QSettings(newConfigLocation, QSettings::IniFormat, this))
{
    if (!QFileInfo::exists(newConfigLocation) && QFileInfo::exists(oldConfigLocation)) {
        QFile::copy(oldConfigLocation, newConfigLocation);
    }

    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        installEventFilter(this);
    }

    connect(m_timer, &QTimer::timeout, this, &PatchManagerObject::quit);
    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::VeryCoarseTimer);
    m_timer->setInterval(15000);  // Try to Quit after 15s timeout
    m_timer->start();
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

void PatchManagerObject::initialize()
{
    getVersion();
    refreshPatchList();

    INotifyWatcher *mainWatcher = new INotifyWatcher(this);
    mainWatcher->addPaths({ PATCHES_DIR });
    connect(mainWatcher, &INotifyWatcher::contentChanged, [this](const QString &path, bool created) {
        qDebug() << "contentChanged:" << path << "created:" << created;
        refreshPatchList();
        emit m_adaptor->patchAltered(path, created);
    });

//    INotifyWatcher *additionalWatcher = new INotifyWatcher(this);
//    additionalWatcher->addPaths({ PATCHES_ADDITIONAL_DIR });
//    connect(additionalWatcher, &INotifyWatcher::contentChanged, [this](const QString &path, bool created) {
//        qDebug() << "contentChanged:" << path << "created:" << created;
//        refreshPatchList();
//    });
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
        return;
    }

    registerDBus();
    if (!m_dbusRegistered) {
        QCoreApplication::exit(2);
        return;
    }

    initialize();
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
    postCustomEvent(PatchManagerEvent::ListPatchesPatchManagerEventType, QVariantMap(), message());
    return QVariantList();
}

QVariantMap PatchManagerObject::listVersions()
{
    qDebug() << Q_FUNC_INFO;
//    m_timer->start();
    QVariantMap versionsList;
    for (const QString &patch : m_metadata.keys()) {
        versionsList[patch] = m_metadata[patch][VERSION_KEY];
    }

    return versionsList;
}

bool PatchManagerObject::isPatchApplied(const QString &patch)
{
    qDebug() << Q_FUNC_INFO;
//    m_timer->start();
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
    postCustomEvent(PatchManagerEvent::ApplyPatchManagerEventType, QVariantMap({{QStringLiteral("name"), patch}}), msg);
    return true;

//    m_timer->stop();

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

//    m_timer->start();
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
    postCustomEvent(PatchManagerEvent::UnapplyPatchManagerEventType, QVariantMap({{QStringLiteral("name"), patch}}), msg);
    return true;

//    m_timer->stop();

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

//    m_timer->start();
    if (ok) {
//        emit m_adaptor->unapplyPatchFinished(patch);
    }
    return ok;
}

bool PatchManagerObject::unapplyAllPatches()
{
    qDebug() << Q_FUNC_INFO;
    return true;

//    m_timer->stop();
    bool ok = true;
    for (const QString &patch : m_appliedPatches.toList()) {
        ok &= unapplyPatch(patch);
    }
//    m_timer->start();
//    emit m_adaptor->unapplyAllPatchesFinished();
    return ok;
}

bool PatchManagerObject::installPatch(const QString &patch, const QString &json, const QString &archive)
{
    qDebug() << Q_FUNC_INFO << patch;
//    m_timer->stop();
    const QString patchPath = QStringLiteral("%1/%2").arg(PATCHES_DIR, patch);
    const QString jsonPath = QStringLiteral("%1/%2").arg(patchPath, PATCH_FILE);
    QFile archiveFile(archive);
    QDir patchDir(patchPath);
    bool result = false;
    if (patchDir.exists()) {
        patchDir.removeRecursively();
    }
    if (archiveFile.exists() && patchDir.mkpath(patchPath)) {
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
//    m_timer->start();
    return result;
}

bool PatchManagerObject::uninstallPatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
//    m_timer->stop();
    if (m_appliedPatches.contains(patch)) {
        unapplyPatch(patch);
//        m_timer->stop();
    }

    QDir patchDir(QString("%1/%2").arg(PATCHES_DIR, patch));
    if (patchDir.exists()) {
        bool ok = patchDir.removeRecursively();
        refreshPatchList();
//        m_timer->start();
        return ok;
    }

//    m_timer->start();
    return true;
}

int PatchManagerObject::checkVote(const QString &patch)
{
    DBUS_GUARD(0)
    qDebug() << Q_FUNC_INFO << patch;
    setDelayedReply(true);
    postCustomEvent(PatchManagerEvent::CheckVotePatchManagerEventType, {{"patch", patch}}, message());
    return 0;
}

void PatchManagerObject::votePatch(const QString &patch, int action)
{
    qDebug() << Q_FUNC_INFO << patch << action;
    postCustomEvent(PatchManagerEvent::VotePatchManagerEventType, {{"patch", patch}, {"action", action}}, message());
}

QString PatchManagerObject::checkEaster()
{
    DBUS_GUARD(QString())
    qDebug() << Q_FUNC_INFO;
    postCustomEvent(PatchManagerEvent::CheckEasterPatchManagerEventType, QVariantMap(), message());
    return QString();
}

QVariantList PatchManagerObject::downloadCatalog(const QVariantMap &params)
{
    DBUS_GUARD(QVariantList())
    qDebug() << Q_FUNC_INFO << params;
    setDelayedReply(true);
    postCustomEvent(PatchManagerEvent::DownloadCatalogPatchManagerEventType, params, message());
    return QVariantList();
}

QVariantMap PatchManagerObject::downloadPatchInfo(const QString &name)
{
    DBUS_GUARD(QVariantMap())
    qDebug() << Q_FUNC_INFO << name;
    setDelayedReply(true);
    postCustomEvent(PatchManagerEvent::DownloadPatchPatchManagerEventType, {{"name", name}}, message());
    return QVariantMap();
}

void PatchManagerObject::checkForUpdates()
{
    DBUS_GUARD()
    qDebug() << Q_FUNC_INFO;
    postCustomEvent(PatchManagerEvent::CheckForUpdatesPatchManagerEventType, QVariantMap(), QDBusMessage());
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
    qDebug() << Q_FUNC_INFO;
    // because QCoreApplication::quit() does note care of us
    QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::Quit));
}

bool PatchManagerObject::eventFilter(QObject *watched, QEvent *event)
{
    if (qEnvironmentVariableIsSet("PM_DEBUG_EVENTFILTER")) {
        qDebug() << watched << event->type();
    }
    if (event->type() == QEvent::Quit && m_havePendingEvent) {
        qWarning() << "Ignoring QEvent::Quit because of pending events";
        return true;
    }
    return QObject::eventFilter(watched, event);
}

void PatchManagerObject::customEvent(QEvent *e)
{
    if (!s_customEventTypes.contains(e->type())) {
        return;
    }
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_havePendingEvent) {
        m_havePendingEvent = false;
    }
    PatchManagerEvent *pmEvent = static_cast<PatchManagerEvent*>(e);
    qDebug() << pmEvent->myEventType << pmEvent->myData;
    switch (pmEvent->myEventType) {
    case PatchManagerEvent::RefreshPatchManagerEventType:
        doRefreshPatchList();
        break;
    case PatchManagerEvent::ListPatchesPatchManagerEventType:
        doListPatches(pmEvent->myMessage);
        break;
    case PatchManagerEvent::ApplyPatchManagerEventType:
        doPatch(pmEvent->myData, pmEvent->myMessage, true);
        break;
    case PatchManagerEvent::UnapplyPatchManagerEventType:
        doPatch(pmEvent->myData, pmEvent->myMessage, false);
        break;
    case PatchManagerEvent::UnapplyAllPatchManagerEventType:
        break;
    case PatchManagerEvent::DownloadCatalogPatchManagerEventType:
        requestDownloadCatalog(pmEvent->myData, pmEvent->myMessage);
        break;
    case PatchManagerEvent::DownloadPatchPatchManagerEventType:
        requestDownloadPatchInfo(pmEvent->myData.value(QStringLiteral("name")).toString(), pmEvent->myMessage);
        break;
    case PatchManagerEvent::CheckForUpdatesPatchManagerEventType:
        requestCheckForUpdates();
        break;
    case PatchManagerEvent::CheckVotePatchManagerEventType:
        doCheckVote(pmEvent->myData.value(QStringLiteral("patch")).toString(), pmEvent->myMessage);
        break;
    case PatchManagerEvent::VotePatchManagerEventType:
        sendVote(pmEvent->myData.value(QStringLiteral("patch")).toString(), pmEvent->myData.value(QStringLiteral("action")).toInt());
        break;
    case PatchManagerEvent::CheckEasterPatchManagerEventType:
        doCheckEaster(pmEvent->myMessage);
        break;
    default:
        qWarning() << "Unhandled event!" << pmEvent->myEventType;
        break;
    }

    m_timer->start();
}

void PatchManagerObject::doRefreshPatchList()
{
    qDebug() << Q_FUNC_INFO;

    // load applied patches

    QFile file (AUSMT_INSTALLED_LIST_FILE);
    if (file.open(QFile::ReadOnly)) {
        while (!file.atEnd()) {
            const QString line = QString::fromLatin1(file.readLine());
            qDebug() << line;
            QStringList splitted = line.split(QChar(' '));
            if (splitted.count() == 2) {
                m_appliedPatches.insert(splitted.first());
            }
        }
        file.close();
    }

    // collect conflicts per file

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
                if (!toPatch.startsWith('/')) {
                    toPatch = toPatch.mid(toPatch.indexOf('/'));
                }
                filesConflicts[toPatch].append(patchFolder);
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

    emit m_adaptor->listPatchesChanged();
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

void PatchManagerObject::doPatch(const QVariantMap &params, const QDBusMessage &message, bool apply)
{
    qDebug() << Q_FUNC_INFO << params << apply;
    const QString &patch = params.value(QStringLiteral("name")).toString();

    QVariantMap patchData = m_metadata[patch];
    QVariant displayName = patchData.contains("display_name") ? patchData["display_name"] : patchData[NAME_KEY];

    QProcess process;
    process.setProgram(apply ? AUSMT_INSTALL : AUSMT_REMOVE);

    QStringList arguments;
    arguments.append(patch);

    process.setArguments(arguments);
    qDebug() << "Starting:" << process.program() << process.arguments();
    process.start();
    process.waitForFinished(-1);

    bool ok = (process.exitCode() == 0);
    qDebug() << "ok:" << ok;
    if (ok) {
        if (apply) {
            m_appliedPatches.insert(patch);
        } else {
            m_appliedPatches.remove(patch);
        }
        refreshPatchList();
    }
    notify(displayName.toString(), apply, ok);

//    m_timer->start();
    if (ok) {
//        emit m_adaptor->applyPatchFinished(patch);
    }

    sendMessageReply(message, ok);

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

    //    m_timer->start();
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

    //    m_timer->start();
        if (ok) {
    //        emit m_adaptor->unapplyPatchFinished(patch);
        }
    //    return ok;
    }
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

void PatchManagerObject::postCustomEvent(PatchManagerEvent::PatchManagerEventType eventType, const QVariantMap &data, const QDBusMessage &message)
{
    m_havePendingEvent = true;
    PatchManagerEvent::post(eventType, this, data, message);
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
    postCustomEvent(PatchManagerEvent::RefreshPatchManagerEventType, QVariantMap(), QDBusMessage());
}

PatchManagerEvent::PatchManagerEvent(PatchManagerEventType eventType, const QVariantMap &data, const QDBusMessage &message)
    : QEvent(QEvent::Type(PatchManagerEvent::customType(eventType)))
    , myEventType(eventType)
    , myData(data)
    , myMessage(message)
{

}

QEvent::Type PatchManagerEvent::customType(PatchManagerEventType eventType)
{
    if (s_customEventTypes[eventType] == QEvent::None)
    {
        s_customEventTypes[eventType] = static_cast<QEvent::Type>(QEvent::registerEventType());
    }
    return s_customEventTypes[eventType];
}

void PatchManagerEvent::post(PatchManagerEventType eventType, QObject *receiver, const QVariantMap &data, const QDBusMessage &message)
{
    QCoreApplication::postEvent(receiver, new PatchManagerEvent(eventType, data, message));
}
