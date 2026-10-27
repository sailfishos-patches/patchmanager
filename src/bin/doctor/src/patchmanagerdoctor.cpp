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
#include "patchmanagerdoctor_adaptor.h"

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

PatchmanagerDoctor::PatchmanagerDoctor(bool dbus, QObject* parent)
        : QObject(parent)
{
    if(dbus) {
        qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO << "Starting with bus";
        m_adaptor = new PatchmanagerDoctorAdaptor(this);
    }
}

bool PatchmanagerDoctor::check(bool verbose = false) { // static

    QStringList report = checkWithReport();
    if (verbose) {
        if (report.count() == 0) {
            QTextStream(stdout) << "No problems to report." << endl;
        } else {
            QTextStream(stdout) << qPrintable(report.join('\n')) << endl;
        }
    }
    return report.count();
}

QStringList PatchmanagerDoctor::checkWithReport() { // static
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

    return report;
}

bool PatchmanagerDoctor::applyBandAid()
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;
    if (check(false) == 0) {
        QTextStream(stdout) << "No problems to fix." << endl;
        return 0;
    }
    bool ok = fixLDPreload(LD_SO_FILE);
    ok = ok && fixFirejail(FIREJAIL_SYS_CONFIG_PATH, PM_FIREJAIL_CONFIG);
    return ok;
}

bool PatchmanagerDoctor::fixLDPreload(const QString& file)
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO << file;

    QFile f(file); // QSaveFile can not read!
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Could not open for reading:" << file << f.errorString();
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
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Could not open for writing:" << file << sf.errorString();
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
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Could not save" << file << sf.errorString();
        return false;
    }
    return true;
}

bool PatchmanagerDoctor::fixFirejail(const QString& systemConfig, const QString& ourConfig)
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO << systemConfig << ourConfig;

    if (!QFile::exists(systemConfig)) {
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Does not exist:" << systemConfig;
        return false;
    }
    if (!QFile::exists(FIREJAIL_OUR_CONFIG_PATH)) {
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Does not exist:" << FIREJAIL_OUR_CONFIG_PATH;
        return false;
    }

    QFile f(systemConfig); // QSaveFile can not read!
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Could not open" << systemConfig << f.errorString();
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
        qCWarning(patchmanagerDoctorLog) << Q_FUNC_INFO << "Could not open" << systemConfig << sf.errorString();
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

void PatchmanagerDoctor::heal()
{
    if(m_adaptor) {
        bool ok = applyBandAid();
        emit m_adaptor->healed(ok);
    }
}
void PatchmanagerDoctor::checkAndNotify()
{
    qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO;

    QDBusConnection connection = QDBusConnection::systemBus();

    if (!connection.registerObject(DBUS_PATH_NAME, this)) {
        qCCritical(patchmanagerDoctorLog) << Q_FUNC_INFO
                << "Could not register DBus object";
    } else {
        qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO << "Bus registered";
    }

    QStringList rpt = checkWithReport();

    emit m_adaptor->problemsDetected(rpt.count());

    if (rpt.count() > 0) {
        emit m_adaptor->problemReport(rpt);
    } else {
        qCDebug(patchmanagerDoctorLog) << Q_FUNC_INFO << "No problems, quit";
        QCoreApplication::quit();
    }
}

