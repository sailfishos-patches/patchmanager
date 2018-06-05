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

#ifndef PATCHMANAGER_H
#define PATCHMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QTranslator>
#include <QtNetwork>
#include <QSettings>
#include <QJSValue>
#include "webdownloader.h"
#include "patchmanagermodel.h"

class QDBusPendingCallWatcher;
class PatchManagerInterface;
class PatchManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverMediaUrl READ serverMediaUrl CONSTANT)
    Q_PROPERTY(bool developerMode READ developerMode WRITE setDeveloperMode NOTIFY developerModeChanged)
    Q_PROPERTY(PatchManagerModel *installedModel READ installedModel CONSTANT)
    Q_PROPERTY(QVariantMap updates READ getUpdates NOTIFY updatesChanged)
    Q_PROPERTY(QStringList updatesNames READ getUpdatesNames NOTIFY updatesChanged)
    Q_PROPERTY(bool appsNeedRestart READ toggleServices NOTIFY toggleServicesChanged)
    Q_PROPERTY(bool failure READ failure NOTIFY failureChanged)
    Q_PROPERTY(QString patchmanagerVersion READ patchmanagerVersion NOTIFY patchmanagerVersionChanged)
    Q_PROPERTY(QString ssuVersion READ ssuVersion NOTIFY ssuVersionChanged)

public:
    explicit PatchManager(QObject *parent = nullptr);
    static PatchManager *GetInstance(QObject *parent = nullptr);
    QString serverMediaUrl();
    bool developerMode();
    void setDeveloperMode(bool developerMode);
    PatchManagerModel *installedModel();
    QString trCategory(const QString &category) const;
    QVariantMap getUpdates() const;
    QStringList getUpdatesNames() const;
    QString patchmanagerVersion() const;
    QString ssuVersion() const;

    bool toggleServices() const;
    bool failure() const;

    Q_INVOKABLE void call(QDBusPendingCallWatcher *call);
    Q_INVOKABLE void watchCall(QDBusPendingCallWatcher *call,
                               QJSValue callback = QJSValue::UndefinedValue,
                               QJSValue errorCallback = QJSValue::UndefinedValue);

    static QVariant unwind(const QVariant &val, int depth = 0);

private slots:
    void requestListPatches(const QString &patch, bool installed);

public slots:
    QDBusPendingCallWatcher *applyPatch(const QString &patch);
    QDBusPendingCallWatcher *unapplyPatch(const QString &patch);
    QDBusPendingCallWatcher *installPatch(const QString &patch, const QString &version, const QString &url);
    QDBusPendingCallWatcher *uninstallPatch(const QString &patch);
    QDBusPendingCallWatcher *resetState(const QString &patch);
//    QDBusPendingCallWatcher *putSettings(const QString &name, const QVariant &value);
//    QDBusPendingCallWatcher *getSettings(const QString &name, const QVariant &def = QVariant());

    QDBusPendingCallWatcher *downloadCatalog(const QVariantMap &params);
    QDBusPendingCallWatcher *downloadPatchInfo(const QString &name);
    QDBusPendingCallWatcher *listVersions();
    QDBusPendingCallWatcher *unapplyAllPatches();
    void restartServices();

    QString patchName(const QString &patch) const;

    bool installTranslator(const QString & patch);
    bool removeTranslator(const QString & patch);
    void activation(const QString & patch, const QString & version);
    int checkVote(const QString &patch);
    void doVote(const QString &patch, int action);
    void checkEaster();
    QString iconForPatch(const QString &patch);
    QString valueIfExists(const QString & filename);

    void checkForUpdates();

    bool putSettingsSync(const QString & name, const QVariant & value);
    void putSettingsAsync(const QString & name, const QVariant & value,
                          QJSValue callback = QJSValue::UndefinedValue,
                          QJSValue errorCallback = QJSValue::UndefinedValue);

    QVariant getSettingsSync(const QString & name, const QVariant & def = QVariant());
    void getSettingsAsync(const QString & name, const QVariant & def = QVariant(),
                          QJSValue callback = QJSValue::UndefinedValue,
                          QJSValue errorCallback = QJSValue::UndefinedValue);

    void onUpdatesAvailable(const QVariantMap &updates);
    void onToggleServicesChanged(bool toggle);
    void onFailureChanged(bool failed);

    void resolveFailure();

signals:
    void easterReceived(const QString & easterText);
    void developerModeChanged(bool developerMode);
    void updatesChanged();
    void toggleServicesChanged(bool toggleServices);
    void failureChanged(bool failed);
    void patchmanagerVersionChanged(const QString &patchmanagerVersion);
    void ssuVersionChanged(const QString &ssuVersion);

private:
    void successCall(QJSValue callback, const QVariant &value);
    void errorCall(QJSValue errorCallback, const QString &message);

    QVariantMap m_updates;

    QHash<QString, QTranslator*> m_translators;
    QNetworkAccessManager *m_nam;

    PatchManagerModel *m_installedModel;
    PatchManagerInterface *m_interface;

    bool m_toggleServices = false;
    bool m_failed = false;

    QString m_patchmanagerVersion;
    QString m_ssuVersion;
};

#endif // PATCHMANAGER_H
