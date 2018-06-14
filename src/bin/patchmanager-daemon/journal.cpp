#include "journal.h"

#include <QVariantMap>
#include <QDebug>
#include <stdio.h>

Journal::Journal(QObject *parent)
    : QObject(parent)
{

}

void Journal::wait()
{
    qDebug() << Q_FUNC_INFO;

    if (m_jw) {
        QMetaObject::invokeMethod(m_jw, "start", Qt::QueuedConnection);
        return;
    }

    m_jw = new JournalWaiter(m_sdj);
    QThread *thread = new QThread(this);
    thread->setObjectName(QStringLiteral("journalWaiter-%1").arg(QDateTime::currentMSecsSinceEpoch()));
    m_jw->moveToThread(thread);

    connect(thread, &QThread::started, m_jw, &JournalWaiter::start);
    connect(m_jw, &JournalWaiter::destroyed, [thread](){
        QMetaObject::invokeMethod(thread, "quit", Qt::QueuedConnection);
    });
    connect(m_jw, &JournalWaiter::pollAgain, [this](){
        QMetaObject::invokeMethod(m_jw, "start", Qt::QueuedConnection);
    });
    connect(m_jw, &JournalWaiter::pollFailed, [this](){
        m_jw->deleteLater();
        m_jw = nullptr;
    });
    connect(m_jw, &JournalWaiter::canProcess, this, &Journal::process);
    thread->start();
}

void Journal::init()
{
    qDebug() << Q_FUNC_INFO;

    if (sd_journal_open(&m_sdj, SD_JOURNAL_LOCAL_ONLY) < 0) {
        return;
    }

    if (sd_journal_get_fd(m_sdj) < 0) {
        perror("Cannot get journal descriptor");
        return;
    }

    sd_journal_add_match(m_sdj, "_EXE=/usr/bin/lipstick", 0);
    sd_journal_seek_tail(m_sdj);

    wait();
}

void Journal::process()
{
    int next_ret = sd_journal_next(m_sdj);

    while (next_ret > 0) {
        const void *data;
        size_t length;
        if (sd_journal_get_data(m_sdj, "MESSAGE", &data, &length) == 0) {
            const QString &message = QString::fromUtf8((const char *)data, length).section(QChar('='), 1);
            if (message.contains(QRegExp("Type.\\w+.unavailable"))) {
                emit matchFound();
            }
        }
        QCoreApplication::processEvents();

        next_ret = sd_journal_next(m_sdj);
    }

    if (next_ret == 0) {
        QMetaObject::invokeMethod(m_jw, "start", Qt::QueuedConnection);
    } else {
        sd_journal_close(m_sdj);
    }
}
