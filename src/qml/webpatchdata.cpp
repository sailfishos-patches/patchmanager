#include "webpatchdata.h"
#include "webcatalog.h"

QString WebPatchData::name() const
{
    return _name;
}

void WebPatchData::setName(const QString &name)
{
    if (_name != name) {
        _name = name;
        emit nameChanged(_name);

        if (_completed) {
            componentComplete();
        }
    }
}

QJsonObject WebPatchData::value() const
{
    return _value;
}

WebPatchData::WebPatchData(QObject * parent) : QObject(parent)
{
    _completed = false;
    _nam = new QNetworkAccessManager(this);
}

void WebPatchData::classBegin()
{

}

void WebPatchData::componentComplete()
{
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", _name);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply * reply = _nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &WebPatchData::serverReply);

    _completed = true;
}

void WebPatchData::serverReply()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            if (reply->bytesAvailable()) {
                QByteArray json = reply->readAll();

                QJsonParseError error;
                QJsonDocument document = QJsonDocument::fromJson(json, &error);

                if (error.error == QJsonParseError::NoError) {
                    _value = document.object();
                    emit valueChanged(_value);
                }
            }
        }
        reply->deleteLater();
    }
}
