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

#include "webpatchesmodel.h"
#include "webcatalog.h"
#include "patchmanager.h"
#include <QDBusPendingReply>
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

    QDBusPendingCallWatcher *watcher = PatchManager::GetInstance()->downloadCatalog(_queryParams);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher){
        QDBusPendingReply<QVariantList> reply = *watcher;
        if (!reply.isError()) {
            const QVariantList catalog = PatchManager::unwind(reply.value()).toList();
            qDebug() << Q_FUNC_INFO << catalog.count();

            beginInsertRows(QModelIndex(), 0, catalog.count() - 1);
            _modelData = catalog;
            endInsertRows();
        }
        watcher->deleteLater();

        _completed = true;
    });
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
