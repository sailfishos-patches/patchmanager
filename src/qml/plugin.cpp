/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
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

#include <QtQml/qqml.h>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QDBusPendingCallWatcher>
#include "patchmanager.h"
#include "webpatchesmodel.h"

// TODO: ask for paypal.me
static QString PAYPAL_DONATE = QStringLiteral("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=R6AJV4U2G33XG");

static QString SOURCE_REPO = QStringLiteral("https://github.com/sailfishos-patches/patchmanager");

static QObject *patchmanager_singleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)
    return PatchManager::GetInstance(engine);
}
static QObject *patchmanagertransalator_singleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)
    return PatchManagerTranslator::GetInstance(engine);
}

/*****  PatchManager ******/
/*! \qmltype PatchManager
    \instantiates PatchManager
    \inqmlmodule org.SfietKonstantin.patchmanager

    \brief Singleton providing access to the \l [C++]{PatchManager} methods and properties.
*/
/*! \qmlproperty string PatchManager::appsToRestart
    List of applications and services that should be restarted after a patch has been activated.
    \sa PatchManager::toggleServicesList
*/
/*! \qmlproperty bool PatchManager::developerMode
    \deprecated
    \sa {Patchmanager Configuration Files}, inifile
*/
/*! \qmlproperty bool PatchManager::failure
    If \c true, PM is in Failure Mode.
*/
/*! \qmlproperty string PatchManager::osVersion
    This property holds the Operating System version. This is used for version checking.
*/
/*! \qmlproperty bool PatchManager::patchDevelMode
    \sa {Patchmanager Configuration Files}, {inifile}{\c{/etc/patchmanager2.conf}}
*/
/*! \qmlproperty string PatchManager::patchmanagerVersion
    This property holds our own version
*/
/*! \qmlproperty string PatchManager::serverMediaUrl
    This poperty holds the URL to download screenshots from.
*/
/*! \qmlproperty int PatchManager::sfosVersionCheck
    This property keeps the setting of VersionCheck
    \sa PatchManagerVersionCheck::CheckMode
*/
/*! \qmlproperty var PatchManager::updates
    Map of internal names and metadata of patches which can be updated.
    \sa PatchManagerObject::getUpdates, {dbus-sys}{D-Bus System Service}
*/
/*! \qmlproperty var PatchManager::updatesNames
    List of display names of patches which can be updated.
    \sa PatchManagerObject::getUpdates, {dbus-sys}{D-Bus System Service}
*/

/*****  PatchManagerTranslator ******/
/*! \qmltype PatchManagerTranslator
    \instantiates PatchManagerTranslator
    \inqmlmodule org.SfietKonstantin.patchmanager

    \brief This Singleton allows patches to include localizations in their shipped QML files.

    To use, add:

    \code
    import org.SfietKonstantin.patchmanager 2.0
    \endcode

    In the root item add:

    \code
    property bool pmTranslationLoaded: PatchManagerTranslator ? PatchManagerTranslator.installTranslator("my-patch-name") : false}
    \endcode

    In the first visible text item replace text with following:

    \code
    pmTranslationLoaded ? qsTr("Translated text") : "Please update patchmanager!"
    \endcode

    If translation is not loaded try using qsTranslate() strings instead:

    \code
    pmTranslationLoaded ? qsTranslate("pm", "Translated text") : "Please update patchmanager!"
    \endcode

*/
/*! \qmlproperty bool PatchManagerTranslator::pmTranslationLoaded
     \c true when the PatchManagerTranslator has been loaded/initialized successfully.
*/

/*****  WebPatchesModel ******/
/*! \qmltype WebPatchesModel
    \instantiates WebPatchesModel
    \inqmlmodule org.SfietKonstantin.patchmanager

    \brief Holds elements from the \l {Patchmanager Web Catalog}{Web Catalog}.
*/
class PatchManagerPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.SfietKonstantin.patchmanager")
public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_ASSERT(strcmp(uri, "org.SfietKonstantin.patchmanager") == 0);
        engine->rootContext()->setContextProperty(QStringLiteral("PAYPAL_DONATE"), PAYPAL_DONATE);
        engine->rootContext()->setContextProperty(QStringLiteral("SOURCE_REPO"), SOURCE_REPO);
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(strcmp(uri, "org.SfietKonstantin.patchmanager") == 0);
        qmlRegisterSingletonType<PatchManager>(uri, 2, 0, "PatchManager", patchmanager_singleton);
        qmlRegisterSingletonType<PatchManagerTranslator>(uri, 2, 0, "PatchManagerTranslator", patchmanagertransalator_singleton);
        qmlRegisterType<WebPatchesModel>(uri, 2, 0, "WebPatchesModel");
        qmlRegisterUncreatableType<QDBusPendingCallWatcher>(uri, 2, 0, "QDBusPendingCallWatcher", "Compatibility import");
        qmlRegisterUncreatableType<PatchManagerVersionCheck>(uri, 2, 0, "VersionCheck", "Not creatable as it is an enum type.");

    }
};

#include "plugin.moc"
