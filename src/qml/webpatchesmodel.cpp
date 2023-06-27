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

#include <algorithm>

/*! \class WebPatchesModel
    \inmodule org.SfietKonstantin.patchmanager
    \brief The WebPatchesModel holds elements from the Web Catalog.
*/
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
    _sorted = false;
    _nam = new QNetworkAccessManager(this);
}

WebPatchesModel::~WebPatchesModel()
{
}

/*! \property WebPatchesModel::sorted
  Whether the model is sorted
*/
/*! \qmlproperty bool WebPatchesModel::sorted
  Whether the model is sorted
*/
/*! \qmlsignal WebPatchesModel::sortedChanged()
  Emitted when \c sorted changed
*/

/*! \property WebPatchesModel::queryParams
  Query parameters
*/
/*! \qmlproperty var WebPatchesModel::queryParams
  Query parameters
*/
/*! \qmlsignal WebPatchesModel::queryParamsChanged()
  Emitted when query parameters change
*/
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
void WebPatchesModel::setSorted(const bool & sorted) {
    if (sorted != _sorted) {
        _sorted = sorted;
        emit WebPatchesModel::sortedChanged();
        if (_completed) {
            componentComplete();
        }
    }
}

/*! \fn void WebPatchesModel::classBegin()
*/
void WebPatchesModel::classBegin()
{

}

QString translateCategory(const QByteArray &category)
{
    return QCoreApplication::translate("Sections", category.data());
}

bool compareStrings(const QString &a, const QString &b)
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

/*! \fn void WebPatchesModel::componentComplete()
*/
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
            QVariantList catalog = PatchManager::unwind(reply.value()).toList();
            qDebug() << Q_FUNC_INFO << catalog.count();

            if (_sorted) {
                const QLatin1String category("category");
                const QLatin1String name("display_name");
                const QByteArray other("other");
                std::sort(catalog.begin(), catalog.end(), [&category, &name, &other](const QVariant &a, const QVariant &b) {
                    const auto amap = a.toMap();
                    const auto bmap = b.toMap();

                    const auto acat = amap[category].toByteArray();
                    const auto bcat = bmap[category].toByteArray();

                    if (acat == bcat) {
                        // If categories are equal then sort by name
                        return compareStrings(amap[name].toString(), bmap[name].toString());
                    }

                    // Move others to the end
                    if (acat == other) {
                        return false;
                    }
                    if (bcat == other) {
                        return true;
                    }

                    // Sort by localized category name
                    return compareStrings(translateCategory(acat), translateCategory(bcat));
                });
            }

            beginInsertRows(QModelIndex(), 0, catalog.count() - 1);
            _modelData = catalog;
            endInsertRows();
        }
        watcher->deleteLater();

        _completed = true;
    });
}

/*! \fn int WebPatchesModel::rowCount(const QModelIndex & parent) const
    \a parent is unused
    Returns the row count
*/
int WebPatchesModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return _modelData.count();
}

/*! \fn QVariant WebPatchesModel::data(const QModelIndex & index, int role) const
 \a index
 \a role
*/
QVariant WebPatchesModel::data(const QModelIndex & index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= _modelData.size())
        return QVariant();
    return _modelData[index.row()].toMap()[_roles[role]];
}
