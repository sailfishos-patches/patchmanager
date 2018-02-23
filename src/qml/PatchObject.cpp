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
    if (m_busy) {
        return;
    }
    setBusy(true);

    if (m_details->value("patched").toBool()) {
        return;
    }
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
    if (m_busy) {
        return;
    }
    setBusy(true);

    if (!m_details->value("patched").toBool()) {
        return;
    }
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

void PatchObject::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged(busy);
}
