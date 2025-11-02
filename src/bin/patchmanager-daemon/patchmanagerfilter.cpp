/*
 * Copyright (C) 2025 Patchmanager for SailfishOS contributors:
 *                   - olf "Olf0" <https://github.com/Olf0>
 *                   - Peter G. "nephros" <sailfish@nephros.org>
 *                   - Vlad G. "b100dian" <https://github.com/b100dian>
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


/*!
 * The current implementation of the filter is a QCache, whole Object contents
 * are not actually used, only the keys are.  Once a file path has been
 * identified as non-existing, it is added to the cache.
 *
 * Checking for presence is done using QCache::object() (or
 * QCache::operator[]), not QCache::contains() in order to have the cache
 * notice "usage" of the cached object.
 *
 * \sa m_filter
 */

#include "patchmanagerfilter.h"

#include <QtCore/QObject>
#include <QStringBuilder>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

PatchManagerFilter::PatchManagerFilter(QObject *parent, int maxCost )
    : QObject(parent)
    , QCache(maxCost)
{
}

/* initialize the "static members", i.e. a list of very frequesntly accessed files. */
/* only use relatively stable sonames here. */
const QStringList PatchManagerFilter::libList = QStringList({
        "/usr/lib64/libpreloadpatchmanager.so",
        "/lib/ld-linux-aarch64.so.1",
        "/lib/ld-linux-armhf.so.3",
        "/lib64/libc.so.6",
        "/lib64/libdl.so.2",
        "/lib64/librt.so.1",
        "/lib64/libpthread.so.0",
        "/lib64/libgcc_s.so.1",
        "/usr/lib64/libtls-padding.so",
        "/usr/lib64/libsystemd.so.0",
        "/usr/lib64/libcap.so.2",
        "/usr/lib64/libmount.so.1",
        "/usr/lib64/libblkid.so.1",
        "/usr/lib64/libgpg-error.so.0"
});
const QStringList PatchManagerFilter::etcList = QStringList({
    "/etc/passwd",
    "/etc/group",
    "/etc/shadow",
    "/etc/localtime",
    "/etc/ld.so.preload",
    "/etc/ld.so.cache",
    "/usr/share/locale/locale.alias"
});

void PatchManagerFilter::setup()
{
    qDebug() << Q_FUNC_INFO;
    // set up cache
    setMaxCost(HOTCACHE_COST_MAX);

    // use a cost of 1 here so they have less chance to be evicted
    foreach(const QString &entry, etcList) {
        if (QFileInfo::exists(entry)) {
            insert(entry, new QObject(), HOTCACHE_COST_STRONG);
        }
    }
    // they may be wrong, so use a higher cost than default
    foreach(const QString &entry, libList) {
        QString libentry(entry);
        if (Q_PROCESSOR_WORDSIZE == 4) { // 32 bit
            libentry.replace("lib64", "lib");
        }

        if (QFileInfo::exists(libentry)) {
            QFileInfo fi(libentry);
            insert(fi.canonicalFilePath(), new QObject(), HOTCACHE_COST_WEAK);
        }
    }
}

//QList<QPair<QString, QVariant>> PatchManagerFilter::stats() const
QString PatchManagerFilter::stats() const
{
    qDebug() << Q_FUNC_INFO;
    QStringList topTen;
    const int ttmax = qEnvironmentVariableIsSet("PM_DEBUG_HOTCACHE") ? size() : 10;
    foreach(const QString &key, keys() ) {
        topTen << key;
        if (topTen.size() >= ttmax)
            break;
    }

    // % instead of + for QStringBuilder
    QString list;
    list % "Filter Stats:"
         % "\n==========================="
         % "\n  Hotcache entries:: .............." % size()
         % "\n  Hotcache cost: .................." % totalCost() + "/" + maxCost()
         % "\n  Hotcache top entries: ..........." % "\n    " + topTen.join("\n    ")
         % "\n===========================";
    return list;
}
