#include <QGuiApplication>
#include <QQuickView>
#include <sailfishapp.h>

#include <QDebug>
#include <QLocale>
#include <QTranslator>

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

    qDebug() << Q_FUNC_INFO << lang;

    return lang;
}

int main(int argc, char *argv[])
{
    qputenv("NO_PM_PRELOAD", "1");

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    QTranslator translator;
    bool success = translator.load(QLocale(getLang()), QStringLiteral("settings-patchmanager"), "-", QStringLiteral("/usr/share/translations/"), ".qm");
    qDebug() << Q_FUNC_INFO << "Translator loaded:" << success;

    success = app->installTranslator(&translator);
    qDebug() << Q_FUNC_INFO << "Translator installed:" << success;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->setSource(QUrl::fromLocalFile(QStringLiteral("/usr/share/patchmanager/data/dialog.qml")));
    view->showFullScreen();

    return app->exec();
}
