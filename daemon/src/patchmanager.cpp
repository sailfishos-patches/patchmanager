#include <QDebug>

#include "patchmanager.h"

static PatchManager* s_pm = nullptr;

static const QString s_patchFolder = QStringLiteral("/usr/share/patchmanager/patches");
static const QString s_patchBackup = QStringLiteral("/tmp/patchmanager/patches");

static const QString s_scriptApply = QStringLiteral("/usr/libexec/pm-apply");
static const QString s_scriptUnapply = QStringLiteral("/usr/libexec/pm-unapply");

static const QString s_configLocation = QStringLiteral("/etc/patchmanager.conf");

static const QString s_sessionBusConnection = QStringLiteral("patchmanagerconnection");


#define NAME(x) #x

PatchManager *PatchManager::GetInstance()
{
    if (!s_pm) {
        s_pm = new PatchManager();
    }
    return s_pm;
}

QString PatchManager::patchFolder()
{
    return s_patchFolder;
}

PatchManager::PatchManager(QObject *parent)
    : QObject(parent)
{

}

bool PatchManager::isReady() const
{
    return m_ready;
}

void PatchManager::refreshPatchList()
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this, NAME(doRefreshPatchList), Qt::QueuedConnection);
}

void PatchManager::doRefreshPatchList()
{

}
