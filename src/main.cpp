/****************************************************************************
 *
 *  main.cpp - Mervncserver, a VNC server for Mer devices
 *  Copyright (C) 2014 Jolla Ltd.
 *  Contact: Reto Zingg <reto.zingg@jolla.com>
 *
 *  This file is part of Mervncserver.
 *
 *  Mervncserver is free software; you can redistribute it and/or modify
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

    if (!configureSignalHandlers()){
        LOG() << "failed to setup Unix Signal Handlers";
        return 1;
    }

    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/100000/dbus/user_bus_socket", 0);

    ScreenToVnc screen2vnc;
    if(!screen2vnc.m_allFine){
        LOG() << "something failed to initialize!";
        return 1;
    }
    return app.exec();
}

