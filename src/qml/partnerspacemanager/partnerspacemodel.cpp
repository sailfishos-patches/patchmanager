/*
 * Copyright (C) 2014 Lucien XU <sfietkonstantin@free.fr>
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

#include "partnerspacemodel.h"
#include "partnerspaceinformation.h"
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <mlite5/MGConfItem>

static const char *PARTNERSPACEMANAGER_DIR = "/usr/share/partnerspacemanager/partnerspaces";
static const char *PARTNERSPACE_FILE = "partnerspace.json";
static const char *NAME_KEY = "name";
static const char *DESCRIPTION_KEY = "description";
// static const char *INFOS_KEY = "infos";
static const char *QML_KEY = "qml";
static const char *LAUNCHER_KEY = "launcher";
static const char *MAIN_QML_FILE = "main.qml";
static const char *PARTNERSPACEMANAGER_LAUNCHER = "/usr/bin/partnerspacemanager-launcher";
static const char *PARTNERSPACE_LAUNCHER = "/usr/bin/partnerspace-launcher";
static const char *PARTNERSPACE_DCONF = "/desktop/jolla/theme/partner_space";
static const char *PARTNERSPACEMANAGER_ID_DCONF = "/desktop/SfietKonstantin/partnerspacemanager/id";
static const char *PARTNERSPACEMANAGER_QML_DCONF = "/desktop/SfietKonstantin/partnerspacemanager/qmlLauncher";
static const char *KILLALL = "/usr/bin/killall";

struct PartnerSpaceModelData
{
    PartnerSpaceModelData()
        : information(0)
    {
    }

    ~PartnerSpaceModelData()
    {
        delete information;
    }

    QString id;
    PartnerSpaceInformation *information;
    QString launcher;
    QString qmlLauncher;
};

PartnerSpaceModel::PartnerSpaceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

PartnerSpaceModel::~PartnerSpaceModel()
{
    qDeleteAll(m_data);
}

QHash<int, QByteArray> PartnerSpaceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(InfoRole, "info");
    roles.insert(AppliedRole, "applied");
    return roles;
}

int PartnerSpaceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

QVariant PartnerSpaceModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= rowCount()) {
        return QVariant();
    }

    const PartnerSpaceModelData *data = m_data.at(row);
    switch (role) {
    case InfoRole:
        return QVariant::fromValue(data->information);
        break;
    case AppliedRole:
        return (data->id == m_currentPartnerSpace);
        break;
    default:
        return QVariant();
        break;
    }
}

int PartnerSpaceModel::count() const
{
    return rowCount();
}

void PartnerSpaceModel::toggle(int row)
{
    if (row < 0 || row >= rowCount()) {
        return;
    }

    PartnerSpaceModelData *data = m_data.at(row);
    // If we tap the same entry as current launcher, we disable partner space
    if (data->id == m_currentPartnerSpace) {
        m_currentPartnerSpace = QString();
    } else {
        m_currentPartnerSpace = data->id;
    }

    // Set the new partnerspace
    MGConfItem partnerSpaceId (PARTNERSPACEMANAGER_ID_DCONF);
    partnerSpaceId.set(m_currentPartnerSpace);
    partnerSpaceId.sync();

    MGConfItem partnerSpace (PARTNERSPACE_DCONF);
    MGConfItem partnerSpaceQml (PARTNERSPACEMANAGER_QML_DCONF);

    // Killall current partnerspace
    QString currentPartnerSpace = partnerSpace.value(QString()).toString();
    if (!currentPartnerSpace.isEmpty()) {
        QFileInfo currentPartnerSpaceInfo(currentPartnerSpace);

        // Killall
        QProcess killall;
        killall.setProgram(KILLALL);

        QStringList args;
        args << currentPartnerSpaceInfo.fileName();
        qDebug() << currentPartnerSpaceInfo.fileName();
        killall.setArguments(args);
        killall.start();
        killall.waitForFinished(-1);
    }


    if (m_currentPartnerSpace.isEmpty()) {
        // Reset partnerspce, remove QML launcher
        partnerSpace.set(QString());
        partnerSpaceQml.unset();

    } else {
        // If we have a launcher, set it
        // if not, we set the QML launcher
        QString launcher = !data->launcher.isEmpty() ? data->launcher : PARTNERSPACEMANAGER_LAUNCHER;
        QString qmlLauncher = !data->launcher.isEmpty() ? QString() : data->qmlLauncher;

        partnerSpace.set(launcher);
        if (!qmlLauncher.isEmpty()) {
            partnerSpaceQml.set(qmlLauncher);
        } else {
            partnerSpaceQml.unset();
        }
    }

    partnerSpace.sync();
    partnerSpaceQml.sync();
    emit dataChanged(index(0), index(rowCount() - 1));

    // Relaunch the partnerspace
    QProcess::startDetached(PARTNERSPACE_LAUNCHER);
}

void PartnerSpaceModel::load()
{
    MGConfItem partnerSpaceConfig (PARTNERSPACEMANAGER_ID_DCONF);
    m_currentPartnerSpace = partnerSpaceConfig.value(QString()).toString();

    QDir root (PARTNERSPACEMANAGER_DIR);
    foreach (const QString &path, root.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
        PartnerSpaceModelData *data = makePartnerSpaceModelData(root, path);
        if (data) {
            m_data.append(data);
        }
    }

    beginInsertRows(QModelIndex(), 0, m_data.count() - 1);
    emit countChanged();
    endInsertRows();
}

PartnerSpaceModelData * PartnerSpaceModel::makePartnerSpaceModelData(const QDir &root, const QString &id)
{
    QDir partnerSpaceDir (root);
    if (!partnerSpaceDir.cd(id)) {
        return 0;
    }

    QFile file (partnerSpaceDir.absoluteFilePath(PARTNERSPACE_FILE));
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return 0;
    }

    const QJsonObject &object = document.object();
    QString name = object.value(NAME_KEY).toString().trimmed();
    QString description = object.value(DESCRIPTION_KEY).toString().trimmed();
    QString launcher;
    QString qmlLauncher;

    if (object.contains(LAUNCHER_KEY)) {
        QString parsedLauncher = object.value(LAUNCHER_KEY).toString().trimmed();
        if (QFile(parsedLauncher).exists()) {
            launcher = parsedLauncher;
        }
    }

    if (launcher.isEmpty() && object.contains(QML_KEY)) {
        QString qmlFile = object.value(QML_KEY).toString().trimmed();
        if (QFile(qmlFile).exists()) {
            qmlLauncher = qmlFile;
        }
    }

    if (launcher.isEmpty() && qmlLauncher.isEmpty()) {
        if (partnerSpaceDir.exists(MAIN_QML_FILE)) {
            qmlLauncher = partnerSpaceDir.absoluteFilePath(MAIN_QML_FILE);
        }
    }

    if (name.isEmpty() || description.isEmpty() || (launcher.isEmpty() && qmlLauncher.isEmpty())) {
        return 0;
    }

    PartnerSpaceModelData *data = new PartnerSpaceModelData;
    data->id = id;
    data->information = PartnerSpaceInformation::create(name, description, this);
    data->launcher = launcher;
    data->qmlLauncher = qmlLauncher;
    return data;
}
