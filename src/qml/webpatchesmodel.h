#ifndef WEBPATCHESMODEL_H
#define WEBPATCHESMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonParseError>

class WebPatchesModel : public QAbstractListModel, public QQmlParserStatus
{
public:
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    Q_PROPERTY(QVariantMap queryParams READ queryParams WRITE setQueryParams NOTIFY queryParamsChanged)
    QVariantMap queryParams() const;
    void setQueryParams(const QVariantMap & queryParams);

    explicit WebPatchesModel(QObject * parent = 0);
    virtual ~WebPatchesModel();

    void classBegin();
    void componentComplete();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames() const { return _roles; }

private:
    QVariantMap _queryParams;
    bool _completed;

    QNetworkAccessManager * _nam;

    QStringList _keys;
    QHash<int, QByteArray> _roles;
    QVariantList _modelData;

private slots:
    void serverReply();
    void onError(QNetworkReply::NetworkError error);

signals:
    void queryParamsChanged(const QVariantMap & queryParams);

};

#endif // WEBPATCHESMODEL_H
