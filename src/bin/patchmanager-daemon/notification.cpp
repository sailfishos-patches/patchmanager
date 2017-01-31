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
#include "notificationmanagerproxy.h"
#include "notification.h"

#include <QStringBuilder>

namespace {

const char *HINT_CATEGORY = "category";
const char *HINT_ITEM_COUNT = "x-nemo-item-count";
const char *HINT_TIMESTAMP = "x-nemo-timestamp";
const char *HINT_PREVIEW_BODY = "x-nemo-preview-body";
const char *HINT_PREVIEW_SUMMARY = "x-nemo-preview-summary";
const char *HINT_REMOTE_ACTION_PREFIX = "x-nemo-remote-action-";
const char *HINT_REMOTE_ACTION_ICON_PREFIX = "x-nemo-remote-action-icon-";
const char *DEFAULT_ACTION_NAME = "default";

//! A proxy for accessing the notification manager
Q_GLOBAL_STATIC_WITH_ARGS(NotificationManagerProxy, notificationManagerProxyInstance, ("org.freedesktop.Notifications", "/org/freedesktop/Notifications", QDBusConnection::sessionBus()))

NotificationManagerProxy *notificationManager()
{
    if (!notificationManagerProxyInstance.exists()) {
        qDBusRegisterMetaType<Notification>();
        qDBusRegisterMetaType<QList<Notification> >();
    }

    return notificationManagerProxyInstance();
}

QString encodeDBusCall(const QString &service, const QString &path, const QString &iface, const QString &method, const QVariantList &arguments)
{
    const QString space(QStringLiteral(" "));

    QString s = service % space % path % space % iface % space % method;

    if (!arguments.isEmpty()) {
        QStringList args;
        int argsLength = 0;

        foreach (const QVariant &arg, arguments) {
            // Serialize the QVariant into a Base64 encoded byte stream
            QByteArray buffer;
            QDataStream stream(&buffer, QIODevice::WriteOnly);
            stream << arg;
            args.append(space + buffer.toBase64());
            argsLength += args.last().length();
        }

        s.reserve(s.length() + argsLength);
        foreach (const QString &arg, args) {
            s.append(arg);
        }
    }

    return s;
}

QStringList encodeActions(const QHash<QString, QString> &actions)
{
    QStringList rv;

    // Actions are encoded as a sequence of name followed by displayName
    QHash<QString, QString>::const_iterator it = actions.constBegin(), end = actions.constEnd();
    for ( ; it != end; ++it) {
        rv.append(it.key());
        rv.append(it.value());
    }

    return rv;
}

QHash<QString, QString> decodeActions(const QStringList &actions)
{
    QHash<QString, QString> rv;

    QStringList::const_iterator it = actions.constBegin(), end = actions.constEnd();
    for ( ; it != end; ++it) {
        // If we have an odd number of tokens, add an empty displayName to complete the last pair
        const QString &name(*it);
        ++it;
        const QString &displayName(it != end ? *it : QString());
        rv.insert(name, displayName);
    }

    return rv;
}

QPair<QHash<QString, QString>, QVariantHash> encodeActionHints(const QVariantList &actions)
{
    QPair<QHash<QString, QString>, QVariantHash> rv;

    foreach (const QVariant &action, actions) {
        QVariantMap vm = action.value<QVariantMap>();
        const QString actionName = vm["name"].value<QString>();
        if (!actionName.isEmpty()) {
            const QString displayName = vm["displayName"].value<QString>();
            const QString service = vm["service"].value<QString>();
            const QString path = vm["path"].value<QString>();
            const QString iface = vm["iface"].value<QString>();
            const QString method = vm["method"].value<QString>();
            const QVariantList arguments = vm["arguments"].value<QVariantList>();
            const QString icon = vm["icon"].value<QString>();

            if (service.isEmpty() || path.isEmpty() || iface.isEmpty() || method.isEmpty()) {
                qWarning() << "Unable to encode invalid remote action:" << action;
            } else {
                rv.first.insert(actionName, displayName);
                rv.second.insert(QString(HINT_REMOTE_ACTION_PREFIX) + actionName, encodeDBusCall(service, path, iface, method, arguments));
                if (!icon.isEmpty()) {
                    rv.second.insert(QString(HINT_REMOTE_ACTION_ICON_PREFIX) + actionName, icon);
                }
            }
        }
    }

    return rv;
}

QVariantList decodeActionHints(const QHash<QString, QString> &actions, const QVariantHash &hints)
{
    QVariantList rv;

    QHash<QString, QString>::const_iterator ait = actions.constBegin(), aend = actions.constEnd();
    for ( ; ait != aend; ++ait) {
        const QString &actionName = ait.key();
        const QString &displayName = ait.value();

        const QString hintName = QString(HINT_REMOTE_ACTION_PREFIX) + actionName;
        const QString &hint = hints[hintName].toString();
        if (!hint.isEmpty()) {
            QVariantMap action;

            // Extract the element of the DBus call
            QStringList elements(hint.split(' ', QString::SkipEmptyParts));
            if (elements.size() <= 3) {
                qWarning() << "Unable to decode invalid remote action:" << hint;
            } else {
                int index = 0;
                action.insert(QStringLiteral("service"), elements.at(index++));
                action.insert(QStringLiteral("path"), elements.at(index++));
                action.insert(QStringLiteral("iface"), elements.at(index++));
                action.insert(QStringLiteral("method"), elements.at(index++));

                QVariantList args;
                while (index < elements.size()) {
                    const QString &arg(elements.at(index++));
                    const QByteArray buffer(QByteArray::fromBase64(arg.toUtf8()));

                    QDataStream stream(buffer);
                    QVariant var;
                    stream >> var;
                    args.append(var);
                }
                action.insert(QStringLiteral("arguments"), args);

                action.insert(QStringLiteral("name"), actionName);
                action.insert(QStringLiteral("displayName"), displayName);

                const QString iconHintName = QString(HINT_REMOTE_ACTION_ICON_PREFIX) + actionName;
                const QString &iconHint = hints[iconHintName].toString();
                if (!iconHint.isEmpty()) {
                    action.insert(QStringLiteral("icon"), iconHint);
                }

                rv.append(action);
            }
        }
    }

    return rv;
}

}

/*!
    \qmltype Notification
    \brief Allows notifications to be published

    The Notification class is a convenience class for using notifications
    based on the
    \l {http://www.galago-project.org/specs/notification/0.9/} {Desktop
    Notifications Specification} as implemented in Nemo.

    Since the Nemo implementation allows static notification parameters,
    such as icon, urgency, priority and user closability definitions, to
    be defined as part of notification category definitions, this
    convenience class is kept as simple as possible by allowing only the
    dynamic parameters, such as summary and body text, item count and
    timestamp to be defined. Other parameters should be defined in the
    notification category definition. Please refer to Lipstick documentation
    for a complete description of the category definition files.

    An example of the usage of this class from a Qml application:

    \qml
    Button {
        Notification {
            id: notification
            category: "x-nemo.example"
            summary: "Notification summary"
            body: "Notification body"
            previewSummary: "Notification preview summary"
            previewBody: "Notification preview body"
            itemCount: 5
            timestamp: "2013-02-20 18:21:00"
            remoteActions: [ {
                "name": "default",
                "displayName": "Do something",
                "icon": "icon-s-do-it",
                "service": "org.nemomobile.example",
                "path": "/example",
                "iface": "org.nemomobile.example",
                "method": "doSomething",
                "arguments": [ "argument", 1 ]
            },{
                "name": "ignore",
                "displayName": "Ignore the problem",
                "icon": "icon-s-ignore",
                "service": "org.nemomobile.example",
                "path": "/example",
                "iface": "org.nemomobile.example",
                "method": "ignore",
                "arguments": [ "argument", 1 ]
            } ]
            onClicked: console.log("Clicked")
            onClosed: console.log("Closed, reason: " + reason)
        }
        text: "Application notification, ID " + notification.replacesId
        onClicked: notification.publish()
    }
    \endqml

    An example category definition file
    /usr/share/lipstick/notificationcategories/x-nemo.example.conf:

    \code
    x-nemo-icon=icon-lock-sms
    x-nemo-preview-icon=icon-s-status-sms
    x-nemo-feedback=sms
    x-nemo-priority=70
    x-nemo-user-removable=true
    x-nemo-user-closeable=false
    \endcode

    After publishing the ID of the notification can be found from the
    replacesId property.

    Notification::notifications() can be used to fetch notifications
    created by the calling application.
 */
Notification::Notification(QObject *parent) :
    QObject(parent),
    appName_(QFileInfo(QCoreApplication::arguments()[0]).fileName()),
    replacesId_(0)
{
    actions_.insert("default", "");
    connect(notificationManager(), SIGNAL(ActionInvoked(uint,QString)), this, SLOT(checkActionInvoked(uint,QString)));
    connect(notificationManager(), SIGNAL(NotificationClosed(uint,uint)), this, SLOT(checkNotificationClosed(uint,uint)));
    connect(this, SIGNAL(remoteDBusCallChanged()), this, SLOT(setRemoteActionHint()));
}

QString Notification::appName() const
{
    return appName_;
}

void Notification::setAppName(const QString &appName)
{
    if (appName != this->appName()) {
        appName_ = appName;
        emit appNameChanged();
    }
}

/*!
    \qmlproperty QString Notification::category

    The type of notification this is. Defaults to an empty string.
 */
QString Notification::category() const
{
    return hints_.value(HINT_CATEGORY).toString();
}

void Notification::setCategory(const QString &category)
{
    if (category != this->category()) {
        hints_.insert(HINT_CATEGORY, category);
        emit categoryChanged();
    }
}

/*!
    \qmlproperty uint Notification::replacesId

    The optional notification ID that this notification replaces. The server must atomically (ie with no flicker or other visual cues) replace the given notification with this one. This allows clients to effectively modify the notification while it's active. A value of value of 0 means that this notification won't replace any existing notifications. Defaults to 0.
 */
uint Notification::replacesId() const
{
    return replacesId_;
}

void Notification::setReplacesId(uint id)
{
    if (replacesId_ != id) {
        replacesId_ = id;
        emit replacesIdChanged();
    }
}

/*!
    \qmlproperty QString Notification::summary

    The summary text briefly describing the notification. Defaults to an empty string.
 */
QString Notification::summary() const
{
    return summary_;
}

void Notification::setSummary(const QString &summary)
{
    if (summary_ != summary) {
        summary_ = summary;
        emit summaryChanged();
    }
}

/*!
    \qmlproperty QString Notification::body

    The optional detailed body text. Can be empty. Defaults to an empty string.
 */
QString Notification::body() const
{
    return body_;
}

void Notification::setBody(const QString &body)
{
    if (body_ != body) {
        body_ = body;
        emit bodyChanged();
    }
}

/*!
    \qmlproperty QDateTime Notification::timestamp

    The timestamp for the notification. Should be set to the time when the event the notification is related to has occurred. Defaults to current time.
 */
QDateTime Notification::timestamp() const
{
    return hints_.value(HINT_TIMESTAMP).toDateTime();
}

void Notification::setTimestamp(const QDateTime &timestamp)
{
    if (timestamp != this->timestamp()) {
        hints_.insert(HINT_TIMESTAMP, timestamp.toString(Qt::ISODate));
        emit timestampChanged();
    }
}

/*!
    \qmlproperty QString Notification::previewSummary

    Summary text to be shown in the preview banner for the notification, if any. Defaults to an empty string.
 */
QString Notification::previewSummary() const
{
    return hints_.value(HINT_PREVIEW_SUMMARY).toString();
}

void Notification::setPreviewSummary(const QString &previewSummary)
{
    if (previewSummary != this->previewSummary()) {
        hints_.insert(HINT_PREVIEW_SUMMARY, previewSummary);
        emit previewSummaryChanged();
    }
}

/*!
    \qmlproperty QString Notification::previewBody

    Body text to be shown in the preview banner for the notification, if any. Defaults to an empty string.
 */
QString Notification::previewBody() const
{
    return hints_.value(HINT_PREVIEW_BODY).toString();
}

void Notification::setPreviewBody(const QString &previewBody)
{
    if (previewBody != this->previewBody()) {
        hints_.insert(HINT_PREVIEW_BODY, previewBody);
        emit previewBodyChanged();
    }
}

/*!
    \qmlproperty int Notification::itemCount

    The number of items represented by the notification. For example, a single notification can represent four missed calls by setting the count to 4. Defaults to 1.
 */
int Notification::itemCount() const
{
    return hints_.value(HINT_ITEM_COUNT).toInt();
}

void Notification::setItemCount(int itemCount)
{
    if (itemCount != this->itemCount()) {
        hints_.insert(HINT_ITEM_COUNT, itemCount);
        emit itemCountChanged();
    }
}

/*!
    \qmlmethod void Notification::publish()

    Publishes the notification. If replacesId is 0, it will be a new
    notification. Otherwise the existing notification with the given ID
    is updated with the new details.
*/
void Notification::publish()
{
    setReplacesId(notificationManager()->Notify(appName_, replacesId_, hints_.value("x-nemo-preview-icon").toString(), summary_, body_, encodeActions(actions_), hints_, -1));
}

/*!
    \qmlmethod void Notification::close()

    Closes the notification if it has been published.
*/
void Notification::close()
{
    if (replacesId_ != 0) {
        notificationManager()->CloseNotification(replacesId_);
        setReplacesId(0);
    }
}

/*!
    \qmlsignal Notification::clicked()

    Sent when the notification is clicked (its default action is invoked).
*/
void Notification::checkActionInvoked(uint id, QString actionKey)
{
    if (id == replacesId_) {
        if (actionKey == DEFAULT_ACTION_NAME) {
            emit clicked();
        }
    }
}

/*!
    \qmlsignal Notification::closed(uint reason)

    Sent when the notification has been closed.
    \a reason is 1 if the notification expired, 2 if the notification was
    dismissed by the user, 3 if the notification was closed by a call to
    CloseNotification.
*/
void Notification::checkNotificationClosed(uint id, uint reason)
{
    if (id == replacesId_) {
        emit closed(reason);
        setReplacesId(0);
    }
}

/*!
    \qmlproperty QString Notification::remoteDBusCallServiceName

    The service name of the D-Bus call for this notification. Defaults to an empty string.
 */
QString Notification::remoteDBusCallServiceName() const
{
    return remoteDBusCallServiceName_;
}

void Notification::setRemoteDBusCallServiceName(const QString &serviceName)
{
    if (remoteDBusCallServiceName_ != serviceName) {
        remoteDBusCallServiceName_ = serviceName;
        emit remoteDBusCallChanged();
    }
}

/*!
    \qmlproperty QString Notification::remoteDBusCallObjectPath

    The object path of the D-Bus call for this notification. Defaults to an empty string.
 */
QString Notification::remoteDBusCallObjectPath() const
{
    return remoteDBusCallObjectPath_;
}

void Notification::setRemoteDBusCallObjectPath(const QString &objectPath)
{
    if (remoteDBusCallObjectPath_ != objectPath) {
        remoteDBusCallObjectPath_ = objectPath;
        emit remoteDBusCallChanged();
    }
}

/*!
    \qmlproperty QString Notification::remoteDBusCallInterface

    The interface of the D-Bus call for this notification. Defaults to an empty string.
 */
QString Notification::remoteDBusCallInterface() const
{
    return remoteDBusCallInterface_;
}

void Notification::setRemoteDBusCallInterface(const QString &interface)
{
    if (remoteDBusCallInterface_ != interface) {
        remoteDBusCallInterface_ = interface;
        emit remoteDBusCallChanged();
    }
}

/*!
    \qmlproperty QString Notification::remoteDBusCallMethodName

    The method name of the D-Bus call for this notification. Defaults to an empty string.
 */
QString Notification::remoteDBusCallMethodName() const
{
    return remoteDBusCallMethodName_;
}

void Notification::setRemoteDBusCallMethodName(const QString &methodName)
{
    if (remoteDBusCallMethodName_ != methodName) {
        remoteDBusCallMethodName_ = methodName;
        emit remoteDBusCallChanged();
    }
}

/*!
    \qmlproperty QVariantList Notification::remoteDBusCallArguments

    The arguments of the D-Bus call for this notification. Defaults to an empty variant list.
 */
QVariantList Notification::remoteDBusCallArguments() const
{
    return remoteDBusCallArguments_;
}

void Notification::setRemoteDBusCallArguments(const QVariantList &arguments)
{
    if (remoteDBusCallArguments_ != arguments) {
        remoteDBusCallArguments_ = arguments;
        emit remoteDBusCallChanged();
    }
}

void Notification::setRemoteActionHint()
{
    QString s;

    if (!remoteDBusCallServiceName_.isEmpty() && !remoteDBusCallObjectPath_.isEmpty() && !remoteDBusCallInterface_.isEmpty() && !remoteDBusCallMethodName_.isEmpty()) {
        s = encodeDBusCall(remoteDBusCallServiceName_,
                           remoteDBusCallObjectPath_,
                           remoteDBusCallInterface_,
                           remoteDBusCallMethodName_,
                           remoteDBusCallArguments_);
    }

    hints_.insert(QString(HINT_REMOTE_ACTION_PREFIX) + DEFAULT_ACTION_NAME, s);
    actions_.insert(DEFAULT_ACTION_NAME, QString());
}

/*!
    \qmlproperty QVariantList Notification::remoteActions

    The remote actions registered for potential invocation by this notification.
 */
QVariantList Notification::remoteActions() const
{
    return remoteActions_;
}

void Notification::setRemoteActions(const QVariantList &remoteActions)
{
    if (remoteActions != remoteActions_) {
        // Remove any existing actions
        foreach (const QVariant &action, remoteActions_) {
            QVariantMap vm = action.value<QVariantMap>();
            const QString actionName = vm["name"].value<QString>();
            if (!actionName.isEmpty()) {
                hints_.remove(QString(HINT_REMOTE_ACTION_PREFIX) + actionName);
                actions_.remove(actionName);
            }
        }

        // Add the new actions and their associated hints
        remoteActions_ = remoteActions;

        QPair<QHash<QString, QString>, QVariantHash> actionHints = encodeActionHints(remoteActions);

        QHash<QString, QString>::const_iterator ait = actionHints.first.constBegin(), aend = actionHints.first.constEnd();
        for ( ; ait != aend; ++ait) {
            actions_.insert(ait.key(), ait.value());
        }

        QVariantHash::const_iterator hit = actionHints.second.constBegin(), hend = actionHints.second.constEnd();
        for ( ; hit != hend; ++hit) {
            hints_.insert(hit.key(), hit.value());
        }

        emit remoteActionsChanged();
    }
}

/*!
    \fn QVariant Notification::hintValue(const QString &hint) const

    Returns the value of the given \a hint .
*/
QVariant Notification::hintValue(const QString &hint) const
{
    return hints_.value(hint);
}

/*!
    \fn void Notification::setHintValue(const QString &hint, const QVariant &value)

    Sets the value of the given \a hint to a given \a value .
*/
void Notification::setHintValue(const QString &hint, const QVariant &value)
{
    hints_.insert(hint, value);
}

/*!
    \qmlmethod void Notification::notifications()

    Returns a list of notifications created by the calling application.
*/
QList<QObject*> Notification::notifications(const QString &appName)
{
    QList<Notification> notifications = notificationManager()->GetNotifications(appName);
    QList<QObject*> objects;
    foreach (const Notification &notification, notifications) {
        objects.append(new Notification(notification));
    }
    return objects;
}

Notification::Notification(const Notification &notification) :
    QObject(notification.parent()),
    replacesId_(notification.replacesId_),
    summary_(notification.summary_),
    body_(notification.body_),
    actions_(notification.actions_),
    hints_(notification.hints_),
    remoteActions_(notification.remoteActions_),
    remoteDBusCallServiceName_(notification.remoteDBusCallServiceName_),
    remoteDBusCallObjectPath_(notification.remoteDBusCallObjectPath_),
    remoteDBusCallInterface_(notification.remoteDBusCallInterface_),
    remoteDBusCallMethodName_(notification.remoteDBusCallMethodName_),
    remoteDBusCallArguments_(notification.remoteDBusCallArguments_)
{
}

QDBusArgument &operator<<(QDBusArgument &argument, const Notification &notification)
{
    argument.beginStructure();
    argument << notification.appName_;
    argument << notification.replacesId_;
    argument << QString();
    argument << notification.summary_;
    argument << notification.body_;
    argument << encodeActions(notification.actions_);
    argument << notification.hints_;
    argument << -1;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Notification &notification)
{
    QString tempString;
    QStringList tempStringList;
    int tempInt;

    argument.beginStructure();
    argument >> tempString;
    argument >> notification.replacesId_;
    argument >> tempString;
    argument >> notification.summary_;
    argument >> notification.body_;
    argument >> tempStringList;
    argument >> notification.hints_;
    argument >> tempInt;
    argument.endStructure();

    notification.actions_ = decodeActions(tempStringList);
    notification.remoteActions_ = decodeActionHints(notification.actions_, notification.hints_);

    return argument;
}
