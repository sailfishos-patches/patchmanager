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

#ifndef PATCHMANAGERFILTER_H
#define PATCHMANAGERFILTER_H

#include <QtCore/QObject>
#include <QCache>

static const int HOTCACHE_COST_MAX = 5000;
static const int HOTCACHE_COST_STRONG  = 1;
static const int HOTCACHE_COST_DEFAULT = 2;
static const int HOTCACHE_COST_WEAK    = 3;

class PatchManagerFilter : public QObject, public QCache<QString, QObject>
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(unsigned int hits READ hits)
    Q_PROPERTY(unsigned int misses READ misses)
public:
    PatchManagerFilter(QObject *parent = nullptr, int maxCost = HOTCACHE_COST_MAX);
    //~PatchManagerFilter();

    void setup();

    // override QCache::contains()
    bool contains(const QString &key) const;

    void setActive(bool active) {
        if (m_active != active) {
            m_active = active;
            emit activeChanged(active);
        }
    };
    bool active() const { return m_active; };

    unsigned int hits()   const { return m_hits; };
    unsigned int misses() const { return m_misses; };

    //QList<QPair<QString, QVariant>> stats() const;
    QString stats() const;

signals:
    void activeChanged(bool);

private:
    bool m_active;
    // need to be mutable so we can count from const method.
    mutable unsigned int m_hits = 0;
    mutable unsigned int m_misses = 0;
};

#endif // PATCHMANAGERFILTER_H
