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
#include "patchmanagermodel.h"
#include "patchmanager.h"

#include <QDBusPendingReply>
#include <QDebug>

PatchManagerModel::PatchManagerModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

PatchManagerModel::PatchManagerModel(const QList<PatchObject *> &data, QObject *parent)
    : QAbstractListModel(parent)
    , m_modelData(data)
{

}

int PatchManagerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_modelData.count();
}

QVariant PatchManagerModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_modelData.size())
        return QVariant();
    if (role == Qt::DisplayRole)
        return m_modelData.at(index.row())->details()->value(QStringLiteral("display_name"));
    if (role == Qt::DecorationRole)
        return m_modelData.at(index.row())->details()->value(QStringLiteral("section"));
    if (role == Qt::EditRole)
        return QVariant::fromValue(m_modelData.at(index.row()));
    return QVariant();
}

QHash<int, QByteArray> PatchManagerModel::roleNames() const
{
    static const QHash<int, QByteArray> r = {
        { Qt::DisplayRole,      QByteArrayLiteral("name") },
        { Qt::DecorationRole,   QByteArrayLiteral("section") },
        { Qt::EditRole,         QByteArrayLiteral("patchObject") },
    };
    return r;
}

QList<PatchObject *> PatchManagerModel::patches() const
{
    return m_modelData;
}

void PatchManagerModel::setPatches(const QList<PatchObject *> &patches)
{
    qDebug() << Q_FUNC_INFO << patches.length();
    beginResetModel();

    for (PatchObject *o : m_modelData) {
        disconnect(o, 0, 0, 0);
    }
    m_modelData = patches;

    m_patchMap.clear();
    for (PatchObject *o : m_modelData) {
        connect(o, &PatchObject::toBeDestroyed, this, &PatchManagerModel::itemRemoved);
        m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
    }

    endResetModel();
}

void PatchManagerModel::populateData(const QVariantList &data, const QString &patch, bool installed)
{
    qDebug() << Q_FUNC_INFO << data.length();
    qDebug() << Q_FUNC_INFO << "Altered:" << patch << "installed:" << installed;

    if (data.isEmpty()) {
        return;
    }

    if (patch.isEmpty()) {
        beginResetModel();
        m_modelData.clear();

        for (const QVariant &var : data) {
            const QVariantMap item = var.toMap();
            PatchObject *o = new PatchObject(item, this);
            connect(o, &PatchObject::toBeDestroyed, this, &PatchManagerModel::itemRemoved);
            m_modelData.append(o);
            m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
        }

        endResetModel();
    } else {
        if (installed) {
            for (const QVariant &var : data) {
                const QVariantMap item = var.toMap();
                if (item.value(QStringLiteral("patch")).toString() == patch) {
                    if (m_patchMap.contains((patch))) {
                        m_patchMap[patch]->setData(item);
                        return;
                    }
                    beginInsertRows(QModelIndex(), m_modelData.count(), m_modelData.count());
                    PatchObject *o = new PatchObject(item, this);
                    connect(o, &PatchObject::toBeDestroyed, this, &PatchManagerModel::itemRemoved);
                    m_modelData.append(o);
                    m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
                    endInsertRows();
                    return;
                }
            }
        } else {
            PatchObject *o = m_patchMap[patch];
            if (!o->details()->value(QStringLiteral("patched")).toBool()) {
                o->deleteLater();
            }
        }
    }
    saveLayout();
}

void PatchManagerModel::removePatch(const QString &patch)
{
    qDebug() << Q_FUNC_INFO << patch;
    if (!m_patchMap.contains(patch)) {
        return;
    }

    PatchObject *p = m_patchMap[patch];
    qDebug() << p;
    int index = m_modelData.indexOf(p);
    qDebug() << index;
    p->deleteLater();
    beginRemoveRows(QModelIndex(), index, index);
    m_modelData.removeAt(index);
    m_patchMap.remove(patch);
    endRemoveRows();
}

void PatchManagerModel::move(int from, int to)
{
    if (from == to) {
        return;
    }
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to < from ? to : (to + 1));
    m_modelData.move(from, to);
    endMoveRows();
}

void PatchManagerModel::saveLayout()
{
    QStringList patches;
    for (PatchObject *o : m_modelData) {
        patches.append(o->details()->value(QStringLiteral("patch")).toString());
    }

    PatchManager::GetInstance()->putSettingsAsync("order", patches);
}

QString PatchManagerModel::patchName(const QString &patch) const
{
    if (!m_patchMap.contains(patch)) {
        return patch;
    }

    return m_patchMap[patch]->details()->value(QStringLiteral("display_name")).toString();
}

void PatchManagerModel::itemRemoved(PatchObject *object)
{
    qDebug() << Q_FUNC_INFO << object;

    if (!object) {
        return;
    }

    if (!m_modelData.contains(object)) {
        return;
    }

    const QString patch = object->details()->value(QStringLiteral("patch")).toString();
    int index = m_modelData.indexOf(object);
    beginRemoveRows(QModelIndex(), index, index);
    m_modelData.removeAt(index);
    m_patchMap.remove(patch);
    endRemoveRows();
}
