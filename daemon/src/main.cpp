#include <QDebug>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTranslator>

#include <unistd.h>

static const QString s_newConfigLocation = QStringLiteral("/etc/patchmanager2.conf");
static const QString s_oldConfigLocation = QStringLiteral("/home/nemo/.config/patchmanager2.conf");

QString getLang()
{
    QString lang = QStringLiteral("en_US.utf8");

    QFile localeConfig(QStringLiteral("/var/lib/environment/nemo/locale.conf"));

    if (!localeConfig.exists() || !localeConfig.open(QFile::ReadOnly)) {
        return lang;
    }

    while (!localeConfig.atEnd()) {
        QString line = localeConfig.readLine().trimmed();
        if (line.startsWith(QStringLiteral("LANG="))) {
             lang = line.mid(5);
             break;
        }
    }

    return lang;
}

int main(int argc, char **argv)
{
    if (getuid() != 0) {
        qWarning() << "Not running as root, exiting";
        return 2;
    }

    qputenv("NO_PM_PRELOAD", "1");

    QCoreApplication app(argc, argv);
#ifdef BUILD_VERSION
    const QString version = QStringLiteral(BUILD_VERSION);
    app.setApplicationVersion(version);
#else
    qDebug() << Q_FUNC_INFO << "Patchmanager version unknown!";
    app.setApplicationVersion(QStringLiteral("3.9.9"));
#endif

    qDebug() << Q_FUNC_INFO << "Patchmanager:" << qApp->applicationVersion();

    const QString lang = getLang();
    qDebug() << Q_FUNC_INFO << "Language:" << lang;

    QTranslator translator;
    qDebug() << Q_FUNC_INFO << "Translator loaded:" <<
        translator.load(QLocale(lang),
                        QStringLiteral("settings-patchmanager"),
                        QStringLiteral("-"),
                        QStringLiteral("/usr/share/translations/"),
                        QStringLiteral(".qm"));

    qDebug() << Q_FUNC_INFO << "Translator installed:" <<
        app.installTranslator(&translator);

    QFile preload(QStringLiteral("/etc/ld.so.preload"));
    if (preload.exists()) {
        qDebug() << Q_FUNC_INFO << "ld.so.preload:";
        if (!preload.open(QFile::ReadOnly)) {
            qWarning() << "Can't open ld.so.preload!";
        }
        qDebug().noquote() << preload.readAll();
    } else {
        qWarning() << Q_FUNC_INFO << "ld.so.preload does not exists!";
    }

    if (!QFileInfo::exists(s_newConfigLocation) && QFileInfo::exists(s_oldConfigLocation)) {
        QFile::copy(s_oldConfigLocation, s_newConfigLocation);
    }

    if (Q_UNLIKELY(qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))) {
        qDebug() << Q_FUNC_INFO << "Injecting DBUS_SESSION_BUS_ADDRESS..." <<
            qputenv("DBUS_SESSION_BUS_ADDRESS", QByteArrayLiteral("unix:path=/run/user/100000/dbus/user_bus_socket"));
    }

    return app.exec();
}
