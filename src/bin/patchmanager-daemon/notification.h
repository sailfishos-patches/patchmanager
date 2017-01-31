/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Vesa Halttunen <vesa.halttunen@jollamobile.com>
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
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QStringList>
#include <QDateTime>
#include <QVariantHash>
#include <QDBusArgument>

class Q_DECL_EXPORT Notification : public QObject
{
    Q_OBJECT

public:
    Notification(QObject *parent = 0);

    Q_PROPERTY(QString appName READ appName WRITE setAppName NOTIFY appNameChanged)
    QString appName() const;
    void setAppName(const QString &appName);

    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    QString category() const;
    void setCategory(const QString &category);

    Q_PROPERTY(uint replacesId READ replacesId WRITE setReplacesId NOTIFY replacesIdChanged)
    uint replacesId() const;
    void setReplacesId(uint id);

    Q_PROPERTY(QString summary READ summary WRITE setSummary NOTIFY summaryChanged)
    QString summary() const;
    void setSummary(const QString &summary);

    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
    QString body() const;
    void setBody(const QString &body);

    Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged)
    QDateTime timestamp() const;
    void setTimestamp(const QDateTime &timestamp);

    Q_PROPERTY(QString previewSummary READ previewSummary WRITE setPreviewSummary NOTIFY previewSummaryChanged)
    QString previewSummary() const;
    void setPreviewSummary(const QString &previewSummary);

    Q_PROPERTY(QString previewBody READ previewBody WRITE setPreviewBody NOTIFY previewBodyChanged)
    QString previewBody() const;
    void setPreviewBody(const QString &previewBody);

    Q_PROPERTY(int itemCount READ itemCount WRITE setItemCount NOTIFY itemCountChanged)
    int itemCount() const;
    void setItemCount(int itemCount);

    Q_PROPERTY(QString remoteDBusCallServiceName READ remoteDBusCallServiceName WRITE setRemoteDBusCallServiceName NOTIFY remoteDBusCallChanged)
    QString remoteDBusCallServiceName() const;
    void setRemoteDBusCallServiceName(const QString &serviceName);

    Q_PROPERTY(QString remoteDBusCallObjectPath READ remoteDBusCallObjectPath WRITE setRemoteDBusCallObjectPath NOTIFY remoteDBusCallChanged)
    QString remoteDBusCallObjectPath() const;
    void setRemoteDBusCallObjectPath(const QString &objectPath);

    Q_PROPERTY(QString remoteDBusCallInterface READ remoteDBusCallInterface WRITE setRemoteDBusCallInterface NOTIFY remoteDBusCallChanged)
    QString remoteDBusCallInterface() const;
    void setRemoteDBusCallInterface(const QString &interface);

    Q_PROPERTY(QString remoteDBusCallMethodName READ remoteDBusCallMethodName WRITE setRemoteDBusCallMethodName NOTIFY remoteDBusCallChanged)
    QString remoteDBusCallMethodName() const;
    void setRemoteDBusCallMethodName(const QString &methodName);

    Q_PROPERTY(QVariantList remoteDBusCallArguments READ remoteDBusCallArguments WRITE setRemoteDBusCallArguments NOTIFY remoteDBusCallChanged)
    QVariantList remoteDBusCallArguments() const;
    void setRemoteDBusCallArguments(const QVariantList &arguments);

    // Obsoletes the set of remoteDBusCall... properties
    Q_PROPERTY(QVariantList remoteActions READ remoteActions WRITE setRemoteActions NOTIFY remoteActionsChanged)
    QVariantList remoteActions() const;
    void setRemoteActions(const QVariantList &remoteActions);
    inline void setRemoteAction(const QVariant &remoteAction) { setRemoteActions(QVariantList() << remoteAction); }

    QVariant hintValue(const QString &hint) const;
    void setHintValue(const QString &hint, const QVariant &value);

    Q_INVOKABLE void publish();
    Q_INVOKABLE void close();
    Q_INVOKABLE static QList<QObject*> notifications(const QString &appName);

    explicit Notification(const Notification &notification);
    friend QDBusArgument &operator<<(QDBusArgument &, const Notification &);
    friend const QDBusArgument &operator>>(const QDBusArgument &, Notification &);

signals:
    void clicked();
    void closed(uint reason);
    void appNameChanged();
    void categoryChanged();
    void replacesIdChanged();
    void summaryChanged();
    void bodyChanged();
    void timestampChanged();
    void previewSummaryChanged();
    void previewBodyChanged();
    void itemCountChanged();
    void remoteActionsChanged();
    void remoteDBusCallChanged();

private slots:
    void checkActionInvoked(uint id, QString actionKey);
    void checkNotificationClosed(uint id, uint reason);
    void setRemoteActionHint();

private:
    QString appName_;
    uint replacesId_;
    QString summary_;
    QString body_;
    QHash<QString, QString> actions_;
    QVariantHash hints_;
    QVariantList remoteActions_;
    QString remoteDBusCallServiceName_;
    QString remoteDBusCallObjectPath_;
    QString remoteDBusCallInterface_;
    QString remoteDBusCallMethodName_;
    QVariantList remoteDBusCallArguments_;
};

Q_DECLARE_METATYPE(Notification)
Q_DECLARE_METATYPE(QList<Notification>)
Q_DECLARE_METATYPE(QList<Notification*>)

#endif // NOTIFICATION_H
