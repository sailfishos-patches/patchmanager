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

#ifndef PATCHMANAGERDOCTOR_PROBLEMS_H
#define PATCHMANAGERDOCTOR_PROBLEMS_H

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
      , DaemonNotRunning
      , DaemonNoPatches
      , DaemonFailed

      , DebugTest = 99
};
static const QMap<Problem, QString> Problems = {
      { Problem::Unknown           , QCoreApplication::translate("Doctor", "Unknown problem") }
    , { Problem::PreloadFile       , QCoreApplication::translate("Doctor", "Preload config does not exist.") }
    , { Problem::PreloadConfig     , QCoreApplication::translate("Doctor", "Preload config is not correct.") }
    , { Problem::JailFile          , QCoreApplication::translate("Doctor", "Firejail config does not exist.") }
    , { Problem::JailConfig        , QCoreApplication::translate("Doctor", "Firejail config is not correct.") }
    , { Problem::NoInetGroup       , QCoreApplication::translate("Doctor", "The system lacks the `inet` group.") }
    , { Problem::NoInetMember      , QCoreApplication::translate("Doctor", "This user is not a member of the `inet` group.") }
    , { Problem::DaemonNotRunning  , QCoreApplication::translate("Doctor", "The Patchmanager Daemon is not running.") }
    , { Problem::DaemonNoPatches   , QCoreApplication::translate("Doctor", "The Patchmanager Daemon has no activated Patches.") }
    , { Problem::DaemonFailed      , QCoreApplication::translate("Doctor", "The Patchmanager Daemon is in failure mode.") }

    , { Problem::DebugTest         , QCoreApplication::translate("Doctor", "THIS IS A TEST PROBLEM") }
};
enum Solution {
        None
      , Reinstall
      , Restart
      , Reboot
};
/*
static const QMap<Solution, QString> Solutions = {
      { Solution::None      , QCoreApplication::translate("Doctor", "Suggested Fix: No known solution") }
    , { Solution::Reinstall , QCoreApplication::translate("Doctor", "Suggested fix: Reinstall Patchmanager.") }
    , { Solution::Restart   , QCoreApplication::translate("Doctor", "Suggested fix: Restart Patchmanager daemon.") }
    , { Solution::Reboot    , QCoreApplication::translate("Doctor", "Suggested fix: Reboot the device.") }
};
*/

#endif // PATCHMANAGERDOCTOR_PROBLEMS_H


