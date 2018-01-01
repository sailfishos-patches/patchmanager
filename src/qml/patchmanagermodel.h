#ifndef PATCHMANAGERMODEL_H
#define PATCHMANAGERMODEL_H

#include <QAbstractListModel>

class PatchManagerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PatchManagerModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const { return m_roles; }

private:
    friend class PatchManager;
    void populateData(const QVariantList &data, const QString &patch, bool installed);

    QList<QVariantMap> m_modelData;
    QStringList m_roleNames;
    QHash<int, QByteArray> m_roles;
};

#endif // PATCHMANAGERMODEL_H
