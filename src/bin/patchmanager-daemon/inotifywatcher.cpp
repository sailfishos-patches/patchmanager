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

#include "inotifywatcher.h"

#include <QFile>
#include <QFileInfo>
#include <QSocketNotifier>

#include <QtGlobal>

#include <QDebug>

#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
// this adds const to non-const objects (like std::as_const)
template <typename T>
Q_DECL_CONSTEXPR typename std::add_const<T>::type &qAsConst(T &t) Q_DECL_NOTHROW { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) Q_DECL_EQ_DELETE;
#endif

INotifyWatcher::INotifyWatcher(QObject *parent)
    : QObject(parent)
{
#if defined(IN_CLOEXEC)
    inotifyFd = inotify_init1(IN_CLOEXEC);
#endif
    if (inotifyFd == -1) {
        inotifyFd = inotify_init();
    }
    notifier = new QSocketNotifier(inotifyFd, QSocketNotifier::Read, this);
    fcntl(inotifyFd, F_SETFD, FD_CLOEXEC);
    connect(notifier, &QSocketNotifier::activated, this, &INotifyWatcher::readFromInotify);
}

/*! \class INotifyWatcher
    \inmodule PatchManagerDaemon
    \inherits QSocketNotifier
    \brief watches a list of files or directories for changes
    \internal
*/
/*! \fn void INotifyWatcher::directoryChanged(const QString &path, bool removed);
    \fn void INotifyWatcher::fileChanged(const QString &path, bool removed);
    This signal is emitted when a file or directory has changed.
    Parameters are the \a path and whether it was \a removed.
 */
/*! \fn void INotifyWatcher::contentChanged(const QString &path, bool created);
    This signal is emitted when directory content has changed.
    Parameters are the \a path and whether something was \a created.
 */
INotifyWatcher::~INotifyWatcher()
{
    notifier->setEnabled(false);
    for (int id : qAsConst(pathToID)) {
        inotify_rm_watch(inotifyFd, id < 0 ? -id : id);
    }
    ::close(inotifyFd);
}

/*! This function adds \a paths to the list of paths to be watched and returns the new list. */
QStringList INotifyWatcher::addPaths(const QStringList &paths)
{
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo fi(path);
        bool isDir = fi.isDir();
        if (isDir) {
            if (directories.contains(path))
                continue;
        } else {
            if (files.contains(path))
                continue;
        }
        int wd = inotify_add_watch(inotifyFd,
                                   QFile::encodeName(path),
                                   (isDir
                                    ? (0
                                       | IN_ATTRIB
                                       | IN_MOVE
                                       | IN_CREATE
                                       | IN_DELETE
                                       | IN_DELETE_SELF
                                       )
                                    : (0
                                       | IN_ATTRIB
                                       | IN_MODIFY
                                       | IN_MOVE
                                       | IN_MOVE_SELF
                                       | IN_DELETE_SELF
                                       )));
        if (wd < 0) {
            qWarning().nospace() << "inotify_add_watch(" << path << ") failed: " << errno;
            continue;
        }
        it.remove();
        int id = isDir ? -wd : wd;
        if (id < 0) {
            directories.append(path);
        } else {
            files.append(path);
        }
        pathToID.insert(path, id);
        idToPath.insert(id, path);
    }
    return p;
}

/*! This function removes \a paths from the list of paths to be watched and returns the new path list.  */
QStringList INotifyWatcher::removePaths(const QStringList &paths)
{
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        int id = pathToID.take(path);
        QString x = idToPath.take(id);
        if (x.isEmpty() || x != path)
            continue;
        int wd = id < 0 ? -id : id;
        // qDebug() << "removing watch for path" << path << "wd" << wd;
        inotify_rm_watch(inotifyFd, wd);
        it.remove();
        if (id < 0) {
            directories.removeAll(path);
        } else {
            files.removeAll(path);
        }
    }
    return p;
}

void INotifyWatcher::readFromInotify()
{
    // qDebug("QInotifyFileSystemWatcherEngine::readFromInotify");
    int buffSize = 0;
    ioctl(inotifyFd, FIONREAD, (char *) &buffSize);
    QVarLengthArray<char, 4096> buffer(buffSize);
    buffSize = read(inotifyFd, buffer.data(), buffSize);
    char *at = buffer.data();
    char * const end = at + buffSize;
    QHash<int, inotify_event *> eventForId;
    while (at < end) {
        inotify_event *event = reinterpret_cast<inotify_event *>(at);
        if (eventForId.contains(event->wd))
            eventForId[event->wd]->mask |= event->mask;
        else
            eventForId.insert(event->wd, event);
        at += sizeof(inotify_event) + event->len;
    }
    QHash<int, inotify_event *>::const_iterator it = eventForId.constBegin();
    while (it != eventForId.constEnd()) {
        const inotify_event &event = **it;
        ++it;
        // qDebug() << "inotify event, wd" << event.wd << "mask" << hex << event.mask;
        int id = event.wd;
        QString path = getPathFromID(id);
        if (path.isEmpty()) {
            // perhaps a directory?
            id = -id;
            path = getPathFromID(id);
            if (path.isEmpty())
                continue;
        }
        // qDebug() << "event for path" << path;

        if ((event.mask & (IN_CREATE | IN_DELETE)) != 0) {
            emit contentChanged(QString::fromUtf8(event.name), (event.mask & (IN_CREATE)) != 0);
        } else if ((event.mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) != 0) {
            pathToID.remove(path);
            idToPath.remove(id, getPathFromID(id));
            if (!idToPath.contains(id))
                inotify_rm_watch(inotifyFd, event.wd);
            if (id < 0)
                emit directoryChanged(path, true);
            else
                emit fileChanged(path, true);
        } else {
            if (id < 0)
                emit directoryChanged(path, false);
            else
                emit fileChanged(path, false);
        }
    }
}

QString INotifyWatcher::getPathFromID(int id) const
{
    QHash<int, QString>::const_iterator i = idToPath.find(id);
    while (i != idToPath.constEnd() && i.key() == id) {
        if ((i + 1) == idToPath.constEnd() || (i + 1).key() != id) {
            return i.value();
        }
        ++i;
    }
    return QString();
}
