#ifndef PATCHMANAGERMODEL_H
#define PATCHMANAGERMODEL_H

#include <QAbstractListModel>

class PatchObject;
class PatchManagerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PatchManagerModel(QObject *parent = nullptr);
    explicit PatchManagerModel(const QList<PatchObject*> &data, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QList<PatchObject*> patches() const;
    void setPatches(const QList<PatchObject*> &patches);
    void populateData(const QVariantList &data, const QString &patch, bool installed);
    void removePatch(const QString &patch);

private:
    Q_DISABLE_COPY(PatchManagerModel)
    friend class PatchManager;

    QList<PatchObject*> m_modelData;
    QMap<QString,PatchObject*> m_patchMap;

};

#endif // PATCHMANAGERMODEL_H
