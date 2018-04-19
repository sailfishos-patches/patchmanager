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

#include "PatchObject.hpp"
#include <patchmanager.h>
#include <QDebug>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

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
    }
    setObjectName(m_details->value(QStringLiteral("patch")).toString());
}

PatchObject::~PatchObject()
{
    qDebug() << Q_FUNC_INFO;
    emit toBeDestroyed(this);
}

QQmlPropertyMap *PatchObject::details()
{
    return m_details;
}

bool PatchObject:: busy() const
{
    return m_busy;
}

void PatchObject::apply()
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
            [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
        } else {
            m_details->setProperty("patched", reply.value());
        }
        setBusy(false);
        watcher->deleteLater();
    });
}

void PatchObject::unapply()
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
            [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            qWarning() << reply.error().type() << reply.error().name() << reply.error().message();
        } else {
            m_details->setProperty("patched", !reply.value());
        }
        setBusy(false);
        watcher->deleteLater();
    });
}

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
            this->deleteLater();
        }
        setBusy(false);
        watcher->deleteLater();
    });
}

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
