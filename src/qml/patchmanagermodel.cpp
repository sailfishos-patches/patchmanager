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

/*! \qmltype PatchManagerModel
    \instantiates PatchManagerModel
    \inqmlmodule org.SfietKonstantin.patchmanager
    \brief A ListModel containing the metadata of Patches.
*/
PatchManagerModel::PatchManagerModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/*! \class PatchManagerModel
    \inheaderfile patchmanagermodel.h
    \inmodule org.SfietKonstantin.patchmanager
    \brief A ListModel containing the metadata of Patches.

    Currently it defines the following Roles:
    \table
      \header
        \li Qt Role
        \li QML Role Name
      \row
        \li \l{https://doc.qt.io/qt-5/qt.html#ItemDataRole-enum}{Qt::DisplayRole}
        \li name
      \row
        \li \l{https://doc.qt.io/qt-5/qt.html#ItemDataRole-enum}{Qt::DecorationRole}
        \li section
      \row
        \li \l{https://doc.qt.io/qt-5/qt.html#ItemDataRole-enum}{Qt::EditRole}
        \li patchObject
    \endtable

    \sa {https://doc.qt.io/qt-5/qabstractitemmodel.html}{Qt::QAbstractItemModel}
*/
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

// /*! \qmlproperty var PatchManagerModel::patches
//     Contains the list of patches
// */
/*!  Returns the list of patches */
QList<PatchObject *> PatchManagerModel::patches() const
{
    return m_modelData;
}

/*!  clears the model data and sets \a patches as new model data.  */
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

/*!
    Does nothing if both \a data and \a patch are empty.

    If \a patch is empty, and we have \a data, extracts the patch name from \a
    data and adds the metadata to the model.

    If we have \a patch and \a data, and \a installed is \c true, add the data
    to the model.

    If we have \a patch and \a data, and \a installed is \c false, just create
    a PatchObject from \a data.

    \warning that last part is not yet documented.

    \sa saveLayout()
*/
void PatchManagerModel::populateData(const QVariantList &data, const QString &patch, bool installed)
{
    qDebug() << Q_FUNC_INFO << data.length();
    qDebug() << Q_FUNC_INFO << "Altered:" << patch << "installed:" << installed;

    if (data.isEmpty() && patch.isEmpty()) {
        return;
    }

    if (patch.isEmpty()) {
        QMap<QString, QVariantMap> patches;
        QStringList order;

        for (const QVariant &var : data) {
            const QVariantMap item = var.toMap();
            const QString patchName = item.value(QStringLiteral("patch")).toString();
            patches.insert(patchName, item);
            order.append(patchName);
        }

        for (const QString &existing : m_patchMap.keys()) {
            PatchObject *o = m_patchMap[existing];
            const int idx = m_modelData.indexOf(o);
            if (!patches.contains(existing)) {
                beginRemoveRows(QModelIndex(), idx, idx);
                o->deleteLater();
                endRemoveRows();
            } else {
                o->setData(patches[existing]);
                dataChanged(index(idx), index(idx));
            }
        }

        for (const QString &changed : order) {
            if (m_patchMap.contains(changed)) {
                continue;
            }

            beginInsertRows(QModelIndex(), m_modelData.length(), m_modelData.length());
            PatchObject *o = new PatchObject(patches[changed], this);
            connect(o, &PatchObject::toBeDestroyed, this, &PatchManagerModel::itemRemoved);
            m_modelData.append(o);
            m_patchMap[changed] = o;
            endInsertRows();
        }

//        beginResetModel();
//        m_modelData.clear();

//        for (const QVariant &var : data) {
//            const QVariantMap item = var.toMap();
//            PatchObject *o = new PatchObject(item, this);
//            connect(o, &PatchObject::toBeDestroyed, this, &PatchManagerModel::itemRemoved);
//            m_modelData.append(o);
//            m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
//        }

//        endResetModel();
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

                    saveLayout();
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

/*!  removes the patch with the name \a patch from the model. */
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

    saveLayout();
}

/*! Moves an entry from index \a from to index \a to */
void PatchManagerModel::move(int from, int to)
{
    if (from == to) {
        return;
    }
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to < from ? to : (to + 1));
    m_modelData.move(from, to);
    endMoveRows();
}

/*! Saves the current order of patch names to Settings. */
void PatchManagerModel::saveLayout()
{
    QStringList patches;
    for (PatchObject *o : m_modelData) {
        patches.append(o->details()->value(QStringLiteral("patch")).toString());
    }

    PatchManager::GetInstance()->putSettingsAsync(QStringLiteral("order"), patches);
}

/*! Returns the \e display_name of \a patch. */
QString PatchManagerModel::patchName(const QString &patch) const
{
    if (!m_patchMap.contains(patch)) {
        return patch;
    }

    return m_patchMap[patch]->details()->value(QStringLiteral("display_name")).toString();
}

/*!  Returns /c true if patch \a name is in the list of applied (activated) patches.  */
bool PatchManagerModel::isApplied(const QString &name) const
{
    // FIXME: there certainly is a more efficient way, e.g. std::find_if?
    for (PatchObject *o : m_modelData) {
        if (o->details()->value(QStringLiteral("name")).toString() == name ) {
            return o->details()->value(QStringLiteral("patched")).toBool();
        }
    }
    return false;
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
