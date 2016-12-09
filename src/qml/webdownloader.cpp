#include "webdownloader.h"
#include "webcatalog.h"

WebDownloader::WebDownloader(QObject *parent) : QObject(parent)
{
    _nam = new QNetworkAccessManager(this);
    _file = new QFile(this);
}

void WebDownloader::start()
{
    _file->setFileName(destination);
    _file->open(QFile::WriteOnly);

    QUrl webUrl(QString(MEDIA_URL"/%1").arg(url));
    QNetworkRequest request(webUrl);
    _reply = _nam->get(request);
    QObject::connect(_reply, &QNetworkReply::finished, this, &WebDownloader::closeFile);
    QObject::connect(_reply, &QNetworkReply::readyRead, this, &WebDownloader::writeBytes);
    QObject::connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void WebDownloader::writeBytes()
{
    if (_file && _file->isOpen()) {
        _file->write(_reply->read(_reply->bytesAvailable()));
    }
}

void WebDownloader::closeFile()
{
    if (_file && _file->isOpen()) {
        _file->close();
    }
    emit downloadFinished(patch, destination);
}

void WebDownloader::error(QNetworkReply::NetworkError)
{
    if (_file && _file->isOpen()) {
        _file->close();
    }
    if (_file->exists()) {
        _file->remove();
    }
    emit downloadError();
}
