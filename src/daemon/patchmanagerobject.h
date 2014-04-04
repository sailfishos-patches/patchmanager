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

#ifndef PATCHMANAGEROBJECT_H
#define PATCHMANAGEROBJECT_H

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

struct PatchInfo
{
    QString workingDirectory;
    QString patchFile;
};

struct Patch
{
    QString patch;
    QString name;
    QString description;
    QString category;
    bool available;
    QVariantMap infos;
    QList<PatchInfo> patchInfos;
};

Q_DECLARE_METATYPE(Patch)
Q_DECLARE_METATYPE(QList<Patch>)

class QDBusInterface;
class PatchManagerObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.SfietKonstantin.patchmanager")
public:
    explicit PatchManagerObject(QObject *parent = 0);
    virtual ~PatchManagerObject();
    void registerDBus();
public slots:
    QList<Patch> listPatches();
    bool isPatchApplied(const QString &patch);
    bool canApplyPatch(const QString &patch);
    bool canUnapplyPatch(const QString &patch);
    bool applyPatch(const QString &patch);
    bool unapplyPatch(const QString &patch);
    void unapplyAllPatches();
    void checkPatches();
    void installLipstickPandora();
    void uninstallLipstickPandora();
    void quit();
protected:
    bool event(QEvent *e);
private:
    void refreshPatchList();
    bool m_dbusRegistered;
    QMap<QString, Patch> m_cachedPatches;
    QMap<QString, Patch> m_cachedAppliedPatches;
    QSet<QString> m_appliedPatches;
    QList<Patch> m_patches;
};

#endif // PATCHMANAGEROBJECT_H

