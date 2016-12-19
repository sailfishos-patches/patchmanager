#include "webpatchesmodel.h"
#include "webcatalog.h"
#include <QDebug>

WebPatchesModel::WebPatchesModel(QObject * parent)
    : QAbstractListModel(parent)
{
    _keys << "description";
    _keys << "last_updated";
    _keys << "name";
    _keys << "display_name";
    _keys << "category";
    _keys << "author";
    _keys << "rating";
    _keys << "total_activations";
    int role = Qt::UserRole + 1;
    foreach (const QString &rolename, _keys) {
        _roles[role++] = rolename.toLatin1();
    }

    _completed = false;
    _nam = new QNetworkAccessManager(this);
}

WebPatchesModel::~WebPatchesModel()
{
}

QVariantMap WebPatchesModel::queryParams() const
{
    return _queryParams;
}

void WebPatchesModel::setQueryParams(const QVariantMap & queryParams)
{
    if (_queryParams != queryParams) {
        _queryParams = queryParams;
        emit queryParamsChanged(_queryParams);

        if (_completed) {
            componentComplete();
        }
    }
}

void WebPatchesModel::classBegin()
{

}

void WebPatchesModel::componentComplete()
{
    if (_modelData.size() > 0) {
        beginRemoveRows(QModelIndex(), 0, _modelData.size() - 1);
        _modelData.clear();
        endRemoveRows();
    }

    QUrl url(CATALOG_URL"/"PROJECTS_PATH);
    QUrlQuery query;
    foreach (const QString & key, _queryParams.keys()) {
        query.addQueryItem(key, _queryParams.value(key).toString());
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = _nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &WebPatchesModel::serverReply);
    QObject::connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &WebPatchesModel::onError);

    _completed = true;
}

int WebPatchesModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return _modelData.count();
}

QVariant WebPatchesModel::data(const QModelIndex & index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= _modelData.size())
        return QVariant();
    return _modelData[index.row()].toMap()[_roles[role]];
}

void WebPatchesModel::serverReply()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            if (reply->bytesAvailable()) {
                QByteArray json = reply->readAll();

                QJsonParseError error;
                QJsonDocument document = QJsonDocument::fromJson(json, &error);

                if (error.error == QJsonParseError::NoError) {
                    QVariantList data = document.toVariant().toList();
                    beginInsertRows(QModelIndex(), 0, data.count() - 1);
                    _modelData = data;
                    endInsertRows();
                }
            }
        }
        reply->deleteLater();
    }
}

void WebPatchesModel::onError(QNetworkReply::NetworkError error)
{
    qDebug() << error;
}
