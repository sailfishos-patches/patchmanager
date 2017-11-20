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

#ifndef PATCHMANAGEROBJECT_H
#define PATCHMANAGEROBJECT_H

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>
#include <QtCore/QVariantList>
#include <QtCore/QDir>

#include <QDBusContext>
#include <QEvent>

#include <QtNetwork/QNetworkAccessManager>

#ifndef SERVER_URL
#define SERVER_URL          "https://coderus.openrepos.net"
#endif

#ifndef API_PATH
#define API_PATH            "pm2/api"
#endif

#define PROJECTS_PATH       "projects"
#define PROJECT_PATH        "project"
#define RATING_PATH         "rating"
#define FILES_PATH          "files"
#define MEDIA_PATH          "media"

#define CATALOG_URL         SERVER_URL"/"API_PATH
#define MEDIA_URL           SERVER_URL"/"MEDIA_PATH

class PatchManagerEvent : public QEvent
{
    Q_GADGET
public:
    enum PatchManagerEventType {
        ApplyPatchManagerEventType,
        UnapplyPatchManagerEventType,
        UnapplyAllPatchManagerEventType,
        DownloadCatalogPatchManagerEventType,
        PatchManagerEventTypeCount
    };
    Q_ENUM(PatchManagerEventType)

    explicit PatchManagerEvent(PatchManagerEventType eventType, const QVariantMap &data);
    static QEvent::Type customType(PatchManagerEventType eventType);
    const PatchManagerEventType myEventType;
    const QVariantMap myData;

    static void post(PatchManagerEventType eventType, QObject *receiver, const QVariantMap &data);
};

class QTimer;
class PatchmanagerAdaptor;
class PatchManagerObject : public QObject, public QDBusContext
{
    Q_OBJECT
public:

    explicit PatchManagerObject(QObject *parent = nullptr);
    virtual ~PatchManagerObject();
    void registerDBus();
public slots:
    QVariantList listPatches();
    QVariantMap listVersions();
    bool isPatchApplied(const QString &patch);
    bool applyPatch(const QString &patch);
    bool unapplyPatch(const QString &patch);
    bool unapplyAllPatches();
    bool installPatch(const QString &patch, const QString &json, const QString &archive);
    bool uninstallPatch(const QString &patch);
    void process();
    void quit();
protected:
    void customEvent(QEvent *e);
private slots:
    void downloadCatalogFinished();
private:
    void requestDownloadCatalog(const QVariantMap &params);

    QVariantList listPatchesFromDir(const QString &dir, QSet<QString> &existingPatches, bool existing = true);
    bool makePatch(const QDir &root, const QString &patchPath, QVariantMap &patch, bool available);
    void notify(const QString &patch, bool apply, bool success);

    void refreshPatchList();
    bool m_dbusRegistered;
    QSet<QString> m_appliedPatches;
    QVariantList m_patches;
    QMap<QString, QVariantMap> m_metadata;
    QTimer *m_timer;
    PatchmanagerAdaptor *m_adaptor;

    QNetworkAccessManager *m_nam;
};

#endif // PATCHMANAGEROBJECT_H

