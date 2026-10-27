/*
 * Copyright (c) 2026 Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
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

#ifndef BAND_AID_H
#define BAND_AID_H

#include <QObject>
#include <QMap>
#include <QCoreApplication>

enum Problem {
        Unknown
      , PreloadFile
      , PreloadConfig
      , JailFile
      , JailConfig
      , NoInetGroup
      , NoInetMember

      , DebugTest = 99
};
static const QMap<Problem, QString> Problems = {
      { Problem::Unknown       , QCoreApplication::translate("SanityCheck", "Unknown problem") }
    , { Problem::PreloadFile   , QCoreApplication::translate("SanityCheck", "Preload config does not exist.") }
    , { Problem::PreloadConfig , QCoreApplication::translate("SanityCheck", "Preload config is not correct.") }
    , { Problem::JailFile      , QCoreApplication::translate("SanityCheck", "Firejail config does not exist.") }
    , { Problem::JailConfig    , QCoreApplication::translate("SanityCheck", "Firejail config is not correct.") }
    , { Problem::NoInetGroup   , QCoreApplication::translate("SanityCheck", "The system lacks the `inet` group.") }
    , { Problem::NoInetMember  , QCoreApplication::translate("SanityCheck", "This user is not a membr of the `inet` group.") }

    , { Problem::DebugTest    , QCoreApplication::translate("SanityCheck", "THIS IS A TEST PROBLEM") }
};
enum Solution {
        None
      , Reinstall
      , Restart
      , Reboot
};
/*
static const QMap<Solution, QString> Solutions = {
      { Solution::None      , QCoreApplication::translate("SanityCheck", "Suggested Fix: No known solution") }
    , { Solution::Reinstall , QCoreApplication::translate("SanityCheck", "Suggested fix: Reinstall Patchmanager.") }
    , { Solution::Restart   , QCoreApplication::translate("SanityCheck", "Suggested fix: Restart Patchmanager daemon.") }
    , { Solution::Reboot    , QCoreApplication::translate("SanityCheck", "Suggested fix: Reboot the device.") }
};
*/


class PatchmanagerDoctor : public QObject
{
    Q_GADGET
public:
    static bool applyBandAid();
    static bool check(bool verbose);
    //static QStringList diagnosis();

public slots:
    void checkAndNotify();

//signals:
//    void selfHealingSuccessful() const;
//    void selfHealingFailed() const;
private:
    static bool fixLDPreload(const QString& file);
    static bool fixFirejail(const QString& systemConfig, const QString& ourConfig);
};


#endif // BAND_AID_H


