/****************************************************************************
 *
 *  logging.h - Lipstick2vnc, a VNC server for Mer devices
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

#ifndef LOGGING_H
#define LOGGING_H

#include <QDebug>
#include <QDateTime>

#define STRINGIFY(z) STRINGIFY2(z)
#define STRINGIFY2(z) #z

#define LOG_HEADER() \
    QDateTime::currentDateTime().toString("hh:mm:ss.zzz")

#define LOG() \
    qDebug() << LOG_HEADER()

#define PRINT(msg) \
    QTextStream(stdout) << "lipstick2vnc: " << msg << endl;

#define IN LOG() << "IN"
#define OUT LOG() << "OUT"

#endif // LOGGING_H
