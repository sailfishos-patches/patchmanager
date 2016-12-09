#ifndef WEBPATCHDATA_H
#define WEBPATCHDATA_H

#include <QObject>
#include <QQmlParserStatus>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

class WebPatchData : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    QString name() const;
    void setName(const QString & name);

    Q_PROPERTY(QJsonObject value READ value NOTIFY valueChanged)
    QJsonObject value() const;

    Q_INVOKABLE void getJson(const QString & version);
    Q_INVOKABLE void reload();

    explicit WebPatchData(QObject * parent = 0);

    void classBegin();
    void componentComplete();

private:
    QString _name;
    QJsonObject _value;
    bool _completed;

    QNetworkAccessManager * _nam;

private slots:
    void serverReply();
    void jsonReply();

signals:
    void nameChanged(const QString & name);
    void valueChanged(const QJsonObject & value);
    void jsonReceived(const QString & json);
    void jsonError();

};

#endif // WEBPATCHDATA_H
