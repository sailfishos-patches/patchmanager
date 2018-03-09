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

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <QtCore/QCoreApplication>
#include "patchmanagerobject.h"
#include <iostream>
#include <QTimer>

void help()
{
    std::cout << "patchmanager-daemon" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  patchmanager-daemon                : run as daemon" << std::endl;
    std::cout << "  patchmanager-daemon -a <patch>     : apply a patch" << std::endl;
    std::cout << "  patchmanager-daemon -u <patch>     : unapply a patch" << std::endl;
    std::cout << "  patchmanager-daemon --unapply-all  : unapply all patches" << std::endl;
    std::cout << "  patchmanager-daemon --daemon       : daemonize" << std::endl;
}

int main(int argc, char **argv)
{
    if (getuid() != 0) {
        fprintf(stderr, "%s: Not running as root, exiting.\n", argv[0]);
        exit(2);
    }

    QCoreApplication app(argc, argv);
    if (app.arguments().length() < 2) {
        help();
        return 0;
    }

    PatchManagerObject patchManager;
    app.installEventFilter(&patchManager);
    QTimer::singleShot(0, &patchManager, &PatchManagerObject::process);
    return app.exec();
}
