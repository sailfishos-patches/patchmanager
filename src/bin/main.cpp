/*
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtQml/qqml.h>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>

static const char *PAYPAL_DONATE = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&"
                                   "hosted_button_id=R6AJV4U2G33XG";

class LipstickPandora: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dumpEnabled READ dumpEnabled WRITE setDumpEnabled NOTIFY dumpEnabledChanged)
public:
    explicit LipstickPandora(QObject *parent = 0);
    Q_INVOKABLE bool isEnabled() const;
    Q_INVOKABLE bool hasDump() const;
    Q_INVOKABLE bool hasInstalled() const;
    bool dumpEnabled() const;
    void setDumpEnabled(bool dumpEnabled);
public slots:
    void restartLipstick();
signals:
    void dumpEnabledChanged();
private:
    bool m_dumpEnabled;
};

LipstickPandora::LipstickPandora(QObject *parent)
    : QObject(parent)
{
    QSettings settings ("SfietKonstantin", "lipstick-pandora");
    m_dumpEnabled = settings.value("dump/enable", false).toBool();
}

bool LipstickPandora::isEnabled() const
{
    QFile file ("/opt/lipstick-pandora/lipstick-pandora");
    return file.exists();
}

bool LipstickPandora::hasDump() const
{
    QDir dir ("/home/nemo/lipstick-pandora");
    return dir.exists();
}

bool LipstickPandora::hasInstalled() const
{
    QDir dir ("/opt/lipstick-pandora/qml");
    return dir.exists();
}

bool LipstickPandora::dumpEnabled() const
{
    return m_dumpEnabled;
}

void LipstickPandora::setDumpEnabled(bool dumpEnabled)
{
    if (m_dumpEnabled != dumpEnabled) {
        QSettings settings ("SfietKonstantin", "lipstick-pandora");
        settings.setValue("dump/enable", dumpEnabled);
        m_dumpEnabled = dumpEnabled;
        emit dumpEnabledChanged();
    }
}

void LipstickPandora::restartLipstick()
{
    QStringList arguments;
    arguments << "--user" << "restart" << "lipstick.service";
    QProcess::startDetached("systemctl", arguments);
}

int main(int argc, char *argv[])
{
    qmlRegisterType<LipstickPandora>("org.SfietKonstantin.patchmanager", 1, 0, "LipstickPandora");
    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();
    view->engine()->rootContext()->setContextProperty("PAYPAL_DONATE", PAYPAL_DONATE);
    view->setSource(QUrl("/usr/share/patchmanager/qml/main.qml"));
    view->show();
    return app->exec();
}

#include "main.moc"
