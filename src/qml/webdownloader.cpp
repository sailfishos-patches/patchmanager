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

#include "webdownloader.h"
#include "webcatalog.h"

/*! \qmltype WebDownloader
    \instantiates WebDownloader
    \inqmlmodule org.SfietKonstantin.patchmanager
    \brief Downloads Patch archives from the Web Catalog
*/
/*! \qmlsignal WebDownloader::downloadFinished(const QString & patch, const QString & fileName)
    This signal is emitted when \a patch has finished downloading, resulting in \a fileName.
*/
/*! \qmlsignal WebDownloader::downloadError()
    This signal is emitted when a download failed.
*/
/*! \class WebDownloader
    \inmodule org.SfietKonstantin.patchmanager
    \brief Downloads Patch archives from the Web Catalog
*/
/*!  \fn void WebDownloader::downloadError()
    This signal is emitted when a download failed.
*/
/*!  \fn void WebDownloader::downloadFinished(const QString &patch, const QString &fileName)
    This signal is emitted when \a patch has finished downloading, resulting in \a fileName.
*/
WebDownloader::WebDownloader(QObject *parent) : QObject(parent)
{
    _nam = new QNetworkAccessManager(this);
    _file = new QFile(this);
}

/*!  starts the download, using compile-time variable \e MEDIA_URL as source.  */
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
/*! \class WebDownloader
    \inmodule org.SfietKonstantin.patchmanager
*/
WebDownloader::WebDownloader(QObject *parent) : QObject(parent)
{
    _nam = new QNetworkAccessManager(this);
    _file = new QFile(this);
}

/*!  Starts the download, usind compile-time variable \e MEDIA_URL, as source and \e as destination file.  */
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
