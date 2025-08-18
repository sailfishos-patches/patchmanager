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
#include "patchmanagerobject.h"
#include <iostream>
#include <QTimer>
#include <QDebug>

#ifndef BUILD_VERSION
#define BUILD_VERSION "99.99.99"
#endif

static void version()
{
    std::cout << "Patchmanager " << BUILD_VERSION << std::endl;
}

static void help()
{
    version();
    std::cout << "Usage:" << std::endl;
    std::cout << "  patchmanager [--help]          : Print this help text" << std::endl;
    std::cout << "  patchmanager -a <Patch>        : Enable and activate a Patch" << std::endl;
    std::cout << "  patchmanager -u <Patch>        : Deactivate and disable (unapply) a Patch" << std::endl;
    std::cout << "  patchmanager --unapply-all     : Deactivate and disable (unapply) all Patches" << std::endl;
    std::cout << "  patchmanager --backup-working  : Save list of enabled Patches as \"working\"" << std::endl;
    std::cout << "  patchmanager --restore-working : Enable backup list of \"working\" Patches" << std::endl;
    std::cout << "  patchmanager --daemon          : Start Patchmanager as daemon" << std::endl;
    std::cout << "  patchmanager --version         : Print the build version and exit." << std::endl;
    std::cout << std::endl;
    std::cout << "Patchmanager must be run as root." << std::endl;
}

int main(int argc, char **argv)
{
    // further argument processing in patchmanagerobject.cpp
    switch (argc) {
      case 1: // we want arguments.
        help(); exit(2); break;
      case 2:
        if (!strcmp(argv[1], "--help")) {
            help(); exit(0);
        } else if (!strcmp(argv[1], "--version")) {
            version(); exit(0);
        }
        break;
    }

    if (getuid() != 0) {
        fprintf(stderr, "%s: Not running as root, exiting.\n\n", argv[0]);
        help();
        exit(2);
    }

    qputenv("NO_PM_PRELOAD", "1");
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QStringLiteral(BUILD_VERSION));

    PatchManagerObject patchManager;
    app.installEventFilter(&patchManager);
    QTimer::singleShot(0, &patchManager, &PatchManagerObject::process);
    return app.exec();
}
