#include <QGuiApplication>
#include <QQuickView>
#include <sailfishapp.h>

#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>
#include <QTimer>

int main(int argc, char *argv[])
{
    qputenv("NO_PM_PRELOAD", "1");

    QSettings pm(QStringLiteral("/etc/patchmanager2.conf"), QSettings::IniFormat);
    if (pm.value(QStringLiteral("settings/applyOnBoot"), false).toBool()) {
        qDebug() << Q_FUNC_INFO << "applyOnBoot is active, exiting!";
        return 0;
    }

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    QTranslator translator;
    bool success = translator.load(QLocale(),
                                   QStringLiteral("settings-patchmanager"),
                                   QStringLiteral("-"),
                                   QStringLiteral("/usr/share/translations/"),
                                   QStringLiteral(".qm"));
    qDebug() << Q_FUNC_INFO << "Translator loaded:" << success;

    success = app->installTranslator(&translator);
    qDebug() << Q_FUNC_INFO << "Translator installed:" << success;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQuickView *v = view.data();

    QTimer::singleShot(0, app.data(), [v]() {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                          QStringLiteral("/org/SfietKonstantin/patchmanager"),
                                                          QStringLiteral("org.SfietKonstantin.patchmanager"),
                                                          QStringLiteral("getLoaded"));
        QDBusReply<bool> reply = QDBusConnection::systemBus().call(msg);
        if (reply.isValid() && !reply.value()) {
            qWarning() << Q_FUNC_INFO << "Showing dialog window";
            v->setSource(QUrl::fromLocalFile(QStringLiteral("/usr/share/patchmanager/data/dialog.qml")));
            v->showFullScreen();
        } else {
            qWarning() << Q_FUNC_INFO << "Exiting!";
            qGuiApp->quit();
        }
    });

    return app->exec();
}
