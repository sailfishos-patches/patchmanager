#include "PatchObject.hpp"
#include "patchmanagermodel.h"

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
    QHash<int, QByteArray> r;
    r[Qt::DisplayRole] = QByteArrayLiteral("name");
    r[Qt::DecorationRole] = QByteArrayLiteral("section");
    r[Qt::EditRole] = QByteArrayLiteral("patchObject");
    return r;
}

QList<PatchObject *> PatchManagerModel::patches() const
{
    return m_modelData;
}

void PatchManagerModel::setPatches(const QList<PatchObject *> &patches)
{
    beginResetModel();
    m_modelData = patches;

    m_patchMap.clear();
    for (PatchObject *o : m_modelData) {
        m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
    }

    endResetModel();
}

void PatchManagerModel::populateData(const QVariantList &data, const QString &patch, bool installed)
{
    qDebug() << Q_FUNC_INFO << data.length();
    qDebug() << "Altered:" << patch << "installed:" << installed;

    if (data.isEmpty()) {
        return;
    }

    if (patch.isEmpty()) {
        beginResetModel();
        m_modelData.clear();

        for (const QVariant &var : data) {
            const QVariantMap item = var.toMap();
            PatchObject *o = new PatchObject(item, this);
            m_modelData.append(o);
            m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
        }

        endResetModel();
    } else {
        PatchObject *o = new PatchObject(data.first().toMap(), this);
        m_modelData.append(o);
        m_patchMap[o->details()->value(QStringLiteral("patch")).toString()] = o;
    }
}

void PatchManagerModel::removePatch(const QString &patch)
{
    if (!m_patchMap.contains(patch)) {
        return;
    }

    PatchObject *p = m_patchMap[patch];
    int index = m_modelData.indexOf(p);
    beginRemoveRows(QModelIndex(), index, index);
    m_modelData.removeAt(index);
    m_patchMap.remove(patch);
    p->deleteLater();
    endRemoveRows();
}
