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

void WebPatchData::getJson(const QString &version)
{
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", _name);
    query.addQueryItem("version", version);
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = _nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &WebPatchData::jsonReply);
}

void WebPatchData::reload()
{
    QUrl url(CATALOG_URL"/"PROJECT_PATH);
    QUrlQuery query;
    query.addQueryItem("name", _name);
    url.setQuery(query);
    QNetworkRequest request(url);
    QNetworkReply * reply = _nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, &WebPatchData::serverReply);
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
    reload();
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

void WebPatchData::jsonReply()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            if (reply->bytesAvailable()) {
                QByteArray json = reply->readAll();

                QJsonParseError error;
                QJsonDocument::fromJson(json, &error);

                if (error.error == QJsonParseError::NoError) {
                    emit jsonReceived(QString::fromUtf8(json));
                } else {
                    emit jsonError();
                }
            }
        }
        reply->deleteLater();
    }
}
