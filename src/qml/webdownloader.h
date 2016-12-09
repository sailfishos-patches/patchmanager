#ifndef WEBDOWNLOADER_H
#define WEBDOWNLOADER_H

#include <QObject>
#include <QtNetwork>
#include <QFile>

class WebDownloader : public QObject
{
    Q_OBJECT
public:
    explicit WebDownloader(QObject *parent = 0);

    QString patch;
    QString url;
    QString destination;

    void start();

private:
    QNetworkAccessManager *_nam;
    QNetworkReply *_reply;
    QFile *_file;

signals:
    void downloadFinished(const QString & patch, const QString & fileName);
    void downloadError();

private slots:
    void writeBytes();
    void closeFile();
    void error(QNetworkReply::NetworkError);
};

#endif // WEBDOWNLOADER_H
