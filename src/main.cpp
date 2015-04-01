/****************************************************************************
 *
 *  main.cpp - Lipstick2vnc, a VNC server for Mer devices
 *  Copyright (C) 2014 Jolla Ltd.
 *  Contact: Reto Zingg <reto.zingg@jolla.com>
 *
 *  This file is part of Lipstick2vnc.
 *
 *  Lipstick2vnc is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 ****************************************************************************/

#include <signal.h>
#include <unistd.h>

#include <QGuiApplication>
#include <QCommandLineParser>

#include "screentovnc.h"
#include "logging.h"


static bool configureSignalHandlers()
{
    struct sigaction hupSignal;

    hupSignal.sa_handler = ScreenToVnc::unixHupSignalHandler;
    sigemptyset(&hupSignal.sa_mask);
    hupSignal.sa_flags = 0;
    hupSignal.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hupSignal, 0) > 0)
        return false;

    LOG() << "Handler for SIGHUP created";

    struct sigaction termSignal;

    termSignal.sa_handler = ScreenToVnc::unixTermSignalHandler;
    sigemptyset(&termSignal.sa_mask);
    termSignal.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &termSignal, 0) > 0)
        return false;

    LOG() << "Handler for SIGTERM created";

    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption scaleOption(QStringList() << "s" << "scale", "scale the image before put to vnc fb", "ratio");
    parser.addOption(scaleOption);

    QCommandLineOption smoothOption(QStringList() << "S" << "smooth", "use smooth Qt option to scale");
    parser.addOption(smoothOption);

    QCommandLineOption usecOption(QStringList() << "u" << "usec", "usec to wait max for events in VNC library", "usec");
    parser.addOption(usecOption);

    QCommandLineOption buffersOption(QStringList() << "b" << "buffers", "how many buffers to create", "buffers");
    parser.addOption(buffersOption);

    QCommandLineOption intervalOption(QStringList() << "p" << "processTimerInterval", "In which interval shall the process timer trigger", "processTimerInterval");
    parser.addOption(intervalOption);

    QCommandLineOption mouseOption(QStringList() << "M" << "no-mouse-handler", "don't handle mouse events from vnc clients");
    parser.addOption(mouseOption);

    parser.process(app);

    bool smoothScaling = parser.isSet(smoothOption);

    int usec = 5000;
    if (parser.isSet(usecOption))
        usec = parser.value(usecOption).toInt();

    float scaleFactor = 1;
    if (parser.isSet(scaleOption))
        scaleFactor = parser.value(scaleOption).toFloat();

    int buffers = 2;
    if (parser.isSet(buffersOption))
        buffers = parser.value(buffersOption).toInt();

    int processTimerInterval = 0;
    if (parser.isSet(intervalOption))
        processTimerInterval = parser.value(intervalOption).toInt();

    bool doMouseHandler = true;
    if (parser.isSet(mouseOption)){
        doMouseHandler = false;
    }

    if (!configureSignalHandlers()){
        LOG() << "failed to setup Unix Signal Handlers";
        return 1;
    }

    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/100000/dbus/user_bus_socket", 0);

    ScreenToVnc screen2vnc(NULL, smoothScaling, scaleFactor, usec, buffers, processTimerInterval, doMouseHandler);
    if(!screen2vnc.m_allFine){
        LOG() << "something failed to initialize!";
        return 1;
    }
    return app.exec();
}

