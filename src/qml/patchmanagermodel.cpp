#include "patchmanagermodel.h"

#include <QDebug>

PatchManagerModel::PatchManagerModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

int PatchManagerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_modelData.count();
}

QVariant PatchManagerModel::data(const QModelIndex &index, int role) const
{

    int row = index.row();
    if (row < 0 || row >= m_modelData.count()) {
        return QVariant();
    }
    return m_modelData[row][m_roleNames[role - Qt::UserRole]];
}

void PatchManagerModel::populateData(const QVariantList &data, const QString &patch, bool installed)
{
    qDebug() << Q_FUNC_INFO << data.length();
    qDebug() << "Altered:" << patch << "installed:" << installed;

    if (patch.isEmpty()) {
        beginResetModel();
        m_modelData.clear();
    }

    for (const QVariant &var : data) {
        const QVariantMap item = var.toMap();
        for (const QString &key : item.keys()) {
            if (!m_roleNames.contains(key)) {
                m_roles[m_roleNames.count() + Qt::UserRole] = key.toLatin1();
                m_roleNames.append(key);
            }
        }
        m_modelData.append(item);
    }

    if (patch.isEmpty()) {
        qDebug() << "Roles:" << m_roleNames;
        endResetModel();
        return;
    }
}
