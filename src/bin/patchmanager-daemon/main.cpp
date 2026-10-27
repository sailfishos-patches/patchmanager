/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 * Copyright (C) 2016 Andrey Kozhevnikov <coderusinbox@gmail.com>
 * Copyright (c) 2021-2023 Patchmanager for SailfishOS contributors:
 *                  - olf "Olf0" <https://github.com/Olf0>
 *                  - Peter G. "nephros" <sailfish@nephros.org>
 *                  - Vlad G. "b100dian" <https://github.com/b100dian>
 *
 * You may use this file under the terms of the 3-clause BSD license,
 * as follows:
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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <QTranslator>
#include <QLocale>

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>

#include "patchmanagerobject.h"
#include <iostream>
#include <QTimer>
#include <QDebug>

#ifndef BUILD_VERSION
#define BUILD_VERSION "99.99.99"
#endif

enum CommandLineParseResult
{
    Done,     // exit 0
    Continue, // no exit
    Error,    // exit 2
};

static CommandLineParseResult handleInitialOptions(QCommandLineParser* parser)
{
    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->addOptions({
        {{ "a", "apply"},     QStringLiteral("Enable and activate <Patch>"),
                              QStringLiteral("Patch")},
        {{ "u", "unapply"},   QStringLiteral("Deactivate and disable <Patch>"),
                              QStringLiteral("Patch")},
    });
    parser->addOptions({
        { QStringLiteral("unapply-all"),      QStringLiteral("Deactivate and disable all Patches")},
        { QStringLiteral("backup-working"),   QStringLiteral("Save list of enabled Patches as \"working\"")},
        { QStringLiteral("restore-working"),  QStringLiteral("Enable backup list of \"working\" Patches")},
#ifdef PM_ENABLE_LEGACY
        { QStringLiteral("reset-system"),     QStringLiteral("Clean the system of all patches, including uninstallation of RPM files. Using this option is unsupported.")},
#endif
        { QStringLiteral("daemon"),           QStringLiteral("Start Patchmanager as daemon")},
    });
    parser->addVersionOption();
    parser->addHelpOption();
    parser->setApplicationDescription(
          QStringLiteral("If run with the --daemon option, run as the main Patchmanager deamon.\n")
        + QStringLiteral("If not run with the --daemon option, act as a client to an existing daemon.\n")
        + QStringLiteral("\nPatchmanager must be run as root.")
    );

    if (!parser->parse(QCoreApplication::arguments())) {
        fprintf(stderr, "%s\n", parser->errorText().toLocal8Bit().constData());
        return CommandLineParseResult::Error;
    }

    if (parser->isSet("version") ) {
        parser->showVersion();
        return CommandLineParseResult::Done;
    }

    if (parser->isSet("help") ) {
        fprintf(stderr, "%s\n", parser->helpText().toLocal8Bit().constData());
        return CommandLineParseResult::Done;
    }

    if (getuid() != 0) {
        fprintf(stderr, "%s\n", parser->helpText().toLocal8Bit().constData());
        fprintf(stderr, "Not called with root pemissions, exiting.\n");
        return CommandLineParseResult::Done;
    }

    return CommandLineParseResult::Continue;
}

int main(int argc, char **argv)
{
    qputenv("NO_PM_PRELOAD", "1");
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QStringLiteral(BUILD_VERSION));

    QCommandLineParser* parser = new QCommandLineParser();
    CommandLineParseResult launch = handleInitialOptions(parser);
    if (launch != CommandLineParseResult::Continue)
        exit((int) launch);

    PatchManagerObject patchManager(parser);
    app.installEventFilter(&patchManager);
    QTimer::singleShot(0, &patchManager, &PatchManagerObject::processArgs);

    return app.exec();
}
