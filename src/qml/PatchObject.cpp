/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021, Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
 *
 * You may use this file under the terms of the 3-clause BSD license,
 * as follows:
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

#include "PatchObject.hpp"
#include <patchmanager.h>
#include <QDebug>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJSEngine>

/*! \qmltype PatchObject
    \instantiates PatchObject
    \inqmlmodule org.SfietKonstantin.patchmanager
    \brief An element to be used as content of an \l {PatchManagerModel}
*/
/*! \fn void PatchObject::toBeDestroyed(PatchObject *object);
    This signal is emitted when \a object is about to be destroyed. (Duh.)
 */
/*! \qmlsignal PatchObject::toBeDestroyed(PatchObject object);
    This signal is emitted when \a object is about to be destroyed. (Duh.)
 */
/*! \qmlsignal PatchObject::busyChanged();
    \internal
 */
PatchObject::PatchObject(const QVariantMap &data, QObject *parent)
    : QObject(parent)
    , m_details(new QQmlPropertyMap(parent))
{
    for (const QString &key : data.keys()) {
        m_details->insert(key, data.value(key));
        m_details->insert(QStringLiteral("section"),
                          PatchManager::GetInstance()->trCategory(m_details->value(QStringLiteral("category")).toString()));
        bool isNewPatch = data.contains(QStringLiteral("display_name"));
        m_details->insert(QStringLiteral("isNewPatch"), isNewPatch);
        if (!isNewPatch) {
            m_details->insert(QStringLiteral("display_name"), m_details->value(QStringLiteral("name")));
        }
        m_details->insert(QStringLiteral("log"), QString());
    }
    setObjectName(m_details->value(QStringLiteral("patch")).toString());
}

/*! \class PatchObject
    \inmodule PatchManager
    \brief An element to be used as content of an \l {PatchManagerModel}
*/
PatchObject::~PatchObject()
{
    qDebug() << Q_FUNC_INFO;
    emit toBeDestroyed(this);
}

/*! \qmlproperty var PatchObject::details

    Holds the Patch metadata.
*/
/*! \property PatchObject::details
 */
QQmlPropertyMap *PatchObject::details()
{
    return m_details;
}

/*! \qmlproperty bool PatchObject::busy

   \c true when an internal operation is in progress.
*/
/*! \property PatchObject::busy
 */
bool PatchObject::busy() const
{
    return m_busy;
}

/*! \fn void PatchObject::setData(const QVariantMap &data)

    Fills the PatchObject's properties from \a data.

    \note If there is a "display_name" field in \a data, it is used. Otherwise, patch name is used.
*/
void PatchObject::setData(const QVariantMap &data)
{
    for (const QString &key : data.keys()) {
        m_details->setProperty(key.toUtf8().constData(), data.value(key));
    }
    m_details->setProperty("section",
                           PatchManager::GetInstance()->trCategory(m_details->value(QStringLiteral("category")).toString()));
    bool isNewPatch = data.contains(QStringLiteral("display_name"));
    m_details->setProperty("isNewPatch", isNewPatch);
    if (!isNewPatch) {
        m_details->setProperty("display_name", m_details->value(QStringLiteral("name")));
    }
}

/*! void PatchObject::apply(QJSValue callback)

    Calls PatchManager::applyPatch with the patch name. If \a callback is callable, calls it afterwards.
    Does nothing if the \c "patched" property is \c true.

    \sa PatchManager::applyPatch
*/
void PatchObject::apply(QJSValue callback)
{
    qDebug() << Q_FUNC_INFO;
    if (m_busy) {
        return;
    }

    if (m_details->value("patched").toBool()) {
        return;
    }

    setBusy(true);
    connect(PatchManager::GetInstance()->applyPatch(m_details->value("patch").toString()),
            &QDBusPendingCallWatcher::finished,
            [this, callback](QDBusPendingCallWatcher *watcher) mutable {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        QVariantMap result;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            result[QStringLiteral("ok")] = false;
            result[QStringLiteral("log")] = reply.error().message();
        } else {
            result = reply.value();
            m_details->setProperty("patched", result.value(QStringLiteral("ok")).toBool());
            m_details->setProperty("log", result.value(QStringLiteral("log")).toString());
        }
        setBusy(false);
        watcher->deleteLater();

        if (callback.isCallable()) {
            QJSValueList callbackArguments;
            callbackArguments << callback.engine()->toScriptValue<QVariantMap>(result);
            callback.call(callbackArguments);
        }
    });
}

/*! \fn void PatchObject::unapply(QJSValue callback)
    Calls PatchManager::unapplyPatch with the patch name. If \a callback is callable, calls it afterwards.
    Does nothing if the \c "patched" property is \c false.

    \sa PatchManager::unapplyPatch
*/
void PatchObject::unapply(QJSValue callback)
{
    qDebug() << Q_FUNC_INFO;
    if (m_busy) {
        return;
    }

    if (!m_details->value("patched").toBool()) {
        return;
    }

    setBusy(true);
    connect(PatchManager::GetInstance()->unapplyPatch(m_details->value("patch").toString()),
            &QDBusPendingCallWatcher::finished,
            [this, callback](QDBusPendingCallWatcher *watcher) mutable {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        QVariantMap result;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
            result[QStringLiteral("ok")] = false;
            result[QStringLiteral("log")] = reply.error().message();
        } else {
            result = reply.value();
            m_details->setProperty("patched", !result.value(QStringLiteral("ok")).toBool());
            m_details->setProperty("log", result.value(QStringLiteral("log")).toString());
        }
        setBusy(false);
        watcher->deleteLater();

        if (callback.isCallable()) {
            QJSValueList callbackArguments;
            callbackArguments << callback.engine()->toScriptValue<QVariantMap>(result);
            callback.call(callbackArguments);
        }
    });
}

/*! \fn void PatchObject::uninstall()
    Calls PatchManager::uninstallPatch with the patch name.

    \sa PatchManager::uninstallPatch
*/
void PatchObject::uninstall()
{
    qDebug() << Q_FUNC_INFO;
    if (m_busy) {
        return;
    }
    setBusy(true);
    connect(PatchManager::GetInstance()->uninstallPatch(m_details->value("patch").toString()),
            &QDBusPendingCallWatcher::finished,
            [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
        } else if (reply.value()) {
//            this->deleteLater();
        }
        setBusy(false);
        watcher->deleteLater();
    });
}

/*! \fn void PatchObject::resetState()
    Calls PatchManager::resetState.

    \sa PatchManager::resetState
*/
void PatchObject::resetState()
{
    qDebug() << Q_FUNC_INFO;
    if (m_busy) {
        return;
    }
    setBusy(true);
    connect(PatchManager::GetInstance()->resetState(m_details->value("patch").toString()),
            &QDBusPendingCallWatcher::finished,
            [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
        } else if (reply.value()) {
            m_details->setProperty("patched", false);
        }
        setBusy(false);
        watcher->deleteLater();
    });
}

void PatchObject::setBusy(bool busy)
{
    qDebug() << Q_FUNC_INFO << busy;
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged(busy);
}
