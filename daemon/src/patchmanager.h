#ifndef PATCHMANAGER_H
#define PATCHMANAGER_H

#include <QObject>

class PatchManager : public QObject
{
    Q_OBJECT
public:
    static PatchManager* GetInstance();
    static QString patchFolder();

    explicit PatchManager(QObject *parent = nullptr);

    bool isReady() const;

    void refreshPatchList();

private slots:
    void doRefreshPatchList();

private:
    bool m_ready = false;

};

#endif // PATCHMANAGER_H
