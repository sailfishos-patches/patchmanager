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

#include "patchmanagerdoctor.h"
#include <stdio.h>
#include <unistd.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>

int main(int argc, char **argv)
{
    qputenv("NO_PM_PRELOAD", "1");

    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(
          QStringLiteral("The doctor checks for and fixes problems with the Patchmanager installation")
    );

    parser.addOptions({
        { { "c", "check"},             QStringLiteral("Check for problems. The return code gives the number of problems found.") },
        { { "l", "list"},              QStringLiteral("Check and list problems") },
        { { "F", "fix-problems"},      QStringLiteral("Try to fix the problem that have been found") },
        { QStringLiteral("dbus"),      QStringLiteral("Notify applicable services of result and problems found. Do not use interactively.") },
    });
    parser.addHelpOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        fprintf(stderr, "%s\n", parser.errorText().toLocal8Bit().constData());
        return -1;
    }

    if (parser.isSet("help") ) {
        fprintf(stderr, "%s\n", parser.helpText().toLocal8Bit().constData());
        return 0;
    }


    if (argc <= 1 || parser.isSet("check") ) {
        int ok = PatchmanagerDoctor::check(false);
        exit(ok);
    } else if (parser.isSet("list")) {
        int ok = PatchmanagerDoctor::check(true);
        exit(ok);
    } else if (parser.isSet("fix-problems")) {
        if (getuid() == 0) {
            exit(PatchmanagerDoctor::applyBandAid());
        } else {
            fprintf(stderr, "You need to be root for this operation. Exiting.\n");
            exit(2);
        }
    }

    //if (getuid() != 0) {
    //    fprintf(stderr, "You need to be root for this operation. Exiting.\n");
    //    exit(2);
    //}

    PatchmanagerDoctor doctor(true);
    QTimer::singleShot(0, &doctor, &PatchmanagerDoctor::checkAndNotify);

    return app.exec();
}
