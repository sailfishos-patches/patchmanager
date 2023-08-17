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

/*! \qmlproperty var PatchObject::details

    The patch \l{metadata} mapped to an QML Object.

    \table
    \header
      \li Name
      \li Type
      \li Description

    \row
    \li \c category
    \li string
    \li Corresponds to the field of the same name in the patch metadata.

    \row
    \li \c display_name
    \li string
    \li Corresponds to the field of the same name in the patch metadata.

    \row
    \li \c name
    \li string
    \li Corresponds to the field of the same name in the patch metadata.

    \row
    \li \c patch
    \li string
    \li Corresponds to the field of the same name in the patch metadata.

    \row
    \li \c section
    \li string
    \li Corresponds to the field of the same name in the patch metadata.

    \row
    \li \c isNewPatch
    \li bool
    \li \c true if this patches metadata is in current format, \c false if it's legacy data.

    \row
    \li \c patched
    \li bool
    \li \c true if this patch is currently applied, \c false otherwise.

    \row
    \li \c ok
    \li bool
    \li \c true if the last apply process succeeded, \c false otherwise.

    \row
    \li \c log
    \li string
    \li Holds the output from the latest patching process.

    \endtable
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
    \inmodule org.SfietKonstantin.patchmanager
    \brief An element to be used as content of an \l {PatchManagerModel}

    Upon construction, the \c data parameter is mapped to PatchObject::details
*/
PatchObject::~PatchObject()
{
    qDebug() << Q_FUNC_INFO;
    emit toBeDestroyed(this);
}

/*! \property PatchObject::details

    Maps the internal property names and values to QML properties.

    See QQmlPropertyMap
 */
QQmlPropertyMap *PatchObject::details()
{
    return m_details;
}

/*! \qmlproperty bool PatchObject::busy

   \c true when an internal operation is in progress.
*/
/*! \property PatchObject::busy

   \c true when an internal operation is in progress.
 */
/*! Returns \c true when an internal operation is in progress. */
bool PatchObject::busy() const
{
    return m_busy;
}

/*!
    Populates the \l{PatchObject::details}{details} property from \a data.

    If there is a \c display_name field in \a data, and \a data is treated as
    regular patch metadata (\c isNewPatch is set to \c true).
    Otherwise, the field \c name is used for display name, and the data is
    treated as legacy metadata.

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

/*!
    Calls PatchManager::applyPatch with the patch name. Calls \a callback afterwards (if callable).

    \note Does nothing if the \c patched property is \c true.
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

/*!
    Calls PatchManager::unapplyPatch with the patch name. Calls \a callback afterwards (if callable).

    \note Does nothing if the \c patched property is \c false.
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

/*! Calls PatchManager::uninstallPatch with the patch name. */
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

/*!  Calls PatchManager::resetState with the patch name. */
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
