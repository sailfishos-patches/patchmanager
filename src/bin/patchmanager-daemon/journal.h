#ifndef JOURNAL_H
#define JOURNAL_H

#include <QObject>
#include <systemd/sd-journal.h>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>

class JournalWaiter : public QObject
{
    Q_OBJECT
public:
    explicit JournalWaiter(sd_journal *sdj)
        : QObject(nullptr)
        , m_sdj(sdj)
    {}

public slots:
    void start() {
        int ret = sd_journal_wait(m_sdj, (uint64_t)-1);
        if (ret < 0) {
            emit pollFailed();
        } else if (ret == SD_JOURNAL_NOP) {
            emit pollAgain();
        } else {
            emit canProcess();
        }
    }

private:
    sd_journal *m_sdj;

signals:
    void pollFailed();
    void canProcess();
    void pollAgain();
};

class Journal : public QObject
{
    Q_OBJECT
public:
    explicit Journal(QObject *parent = nullptr);

private:
    sd_journal *m_sdj = nullptr;
    JournalWaiter *m_jw = nullptr;

signals:
    void matchFound();

public slots:
    void init();
    void process();

private slots:
    void wait();
};

#endif // JOURNAL_H
