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

#include "patchmanagerdoctor.h"
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QSaveFile>

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
//#include <QtDBus/QDBusInterface>
//#include <QtDBus/QDBusMetaType>
//#include <QtDBus/QDBusVariant>
//#include <QtDBus/QDBusConnectionInterface>

#include "grp.h"

#include "common/loggingcategory.h"
Q_LOGGING_CATEGORY(patchmanagerDoctorLog, "patchmanager.doctor");

static const QString LD_SO_FILE = QStringLiteral("/etc/ld.so.preload");
static const QString FIREJAIL_OUR_CONFIG_PATH = QStringLiteral("/etc/firejail/whitelist-common-patchmanager.local");
static const QString FIREJAIL_SYS_CONFIG_PATH = QStringLiteral("/etc/firejail/whitelist-common.local");
static const QString FIREJAIL_CONFIG_STRING   = QStringLiteral("include whitelist-common-patchmanager.local");

static const QString DBUS_SERVICE_NAME = QStringLiteral("org.SfietKonstantin.patchmanager");
static const QString DBUS_PATH_NAME    = QStringLiteral("/org/SfietKonstantin/patchmanagerDoctor");
static const QString DBUS_IFACE_NAME   = QStringLiteral("org.SfietKonstantin.patchmanagerDoctor");

static const QString DBUS_CHECK_SIGNAL_NAME  = QStringLiteral("checkResult");
static const QString DBUS_FIX_SIGNAL_NAME    = QStringLiteral("fixResult");
static const QString DBUS_REPORT_SIGNAL_NAME = QStringLiteral("checkReport");

static const char PM_PRELOAD_LIB[]  = "libpreloadpatchmanager.so";
static const char PM_FIREJAIL_CONFIG[] = "whitelist-common-patchmanager.local";

static QStringList getAllGroups() {
    QStringList out;
    group *gr;
    while ((gr = getgrent()) != nullptr) {
        out.append(gr->gr_name);
    }
    endgrent();
    return out;
}


bool PatchmanagerDoctor::check(bool verbose) { // static
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;

    QStringList report;
    //QString tofix = Solutions.value(Solution::None);
    if(!QFile::exists(LD_SO_FILE)) {
      report << Problems.value(Problem::PreloadFile);
      //tofix = Solutions.value(Solution::Reinstall);
    } else {
        bool ok = false;
        QFile file(LD_SO_FILE);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in (&file);
        QString line;
        do {
            line = in.readLine();
            if (line.contains(QString::fromLatin1(PM_PRELOAD_LIB), Qt::CaseSensitive)) {
                ok = true;
                file.close();
                break;
            }
        } while (!line.isNull());
        if(!ok) {
            report << Problems.value(Problem::PreloadConfig);
            //tofix = Solutions.value(Solution::Reinstall);
        }
    }

    // TODO: check system config existance
    if(!QFile::exists(FIREJAIL_OUR_CONFIG_PATH)) {
      report << Problems.value(Problem::JailFile);
      //tofix = Solutions.value(Solution::Reinstall);
    } else {
        bool ok = false;
        QFile file(FIREJAIL_SYS_CONFIG_PATH);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in (&file);
        QString line;
        do {
            line = in.readLine();
            if (line.contains(FIREJAIL_CONFIG_STRING, Qt::CaseSensitive)) {
                ok = true;
                file.close();
                break;
            }
        } while (!line.isNull());
        if(!ok) {
            report << Problems.value(Problem::JailConfig);
            //tofix = Solutions.value(Solution::Reinstall);
        }
    }
    if(!getAllGroups().contains("inet"))
        report << Problems.value(Problem::NoInetGroup);
    /*
    if(!getGroups().contains("inet"))
        report << Problems.value(Problem::NoInetMember);
    */

    if (verbose) {
        if (report.count() == 0) {
            QTextStream(stdout) << "No problems to report." << endl;
        } else {
            QTextStream(stdout) << qPrintable(report.join('\n')) << endl;
        }
    }
    return report.count();
}

bool PatchmanagerDoctor::applyBandAid()
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;
    bool ok = fixLDPreload(LD_SO_FILE);
    ok = ok && fixFirejail(FIREJAIL_SYS_CONFIG_PATH, PM_FIREJAIL_CONFIG);
    return ok;
}

bool PatchmanagerDoctor::fixLDPreload(const QString& file)
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;
    QFile f(file); // QSaveFile can not read!
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //m_error = QStringLiteral("Could not open %1").arg(file);
        return false;
    }

    const QByteArray contents = f.readAll();
    if(contents.contains(QByteArray(PM_PRELOAD_LIB))) {  // nothing to fix
        f.close();
        return true;
    }
    f.close();

    QSaveFile sf(file);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //m_error = QStringLiteral("Could not open %1").arg(file);
        return false;
    }
    QTextStream out(&sf);
    out << contents;
    out << "/usr";
    if (Q_PROCESSOR_WORDSIZE == 4) { // 32 bit
        out << "/lib";
    } else {
        out << "/lib64";
    }
    out << "/" << PM_PRELOAD_LIB << "\n";
    out.flush();

    if(!sf.commit()) {
        //m_error = QStringLiteral("Could not save %1").arg(file);
        return false;
    }
    return true;
}

bool PatchmanagerDoctor::fixFirejail(const QString& systemConfig, const QString& ourConfig)
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;
    if (!QFile::exists(systemConfig)) {
        //m_error = QStringLiteral("Does not exist: %1").arg(systemConfig);
        return false;
    }
    if (!QFile::exists(FIREJAIL_OUR_CONFIG_PATH)) {
        //m_error = QStringLiteral("Does not exist: %1").arg(FIREJAIL_OUR_CONFIG_PATH);
        return false;
    }

    QFile f(systemConfig); // QSaveFile can not read!
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //m_error = QStringLiteral("Could not open %1").arg(systemConfig);
        return false;
    }

    const QByteArray contents = f.readAll();
    if(contents.contains(ourConfig.toLatin1())) {  // nothing to fix
        f.close();
        return true;
    }
    f.close();

    QSaveFile sf(systemConfig);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //m_error = QStringLiteral("Could not open %1").arg(systemConfig);
        return false;
    }

    QTextStream out(&sf);
    out << contents;
    out << "include " << PM_FIREJAIL_CONFIG << "\n";
    out.flush();

    if(!sf.commit()) {
        //m_error = QStringLiteral("Could not save %1").arg(systemConfig);
        return false;
    }
    return true;
}

void PatchmanagerDoctor::checkAndNotify()
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;
    int count = check(false);
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage msg = QDBusMessage::createSignal(
                    DBUS_PATH_NAME, DBUS_IFACE_NAME, DBUS_CHECK_SIGNAL_NAME
                    );
    msg.setArguments ( { QVariant(count) } );
    connection.send(msg);
}


