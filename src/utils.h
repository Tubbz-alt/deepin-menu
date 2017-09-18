/*
 * Copyright (C) 2015 ~ 2017 Deepin Technology Co., Ltd.
 *
 * Author:     Hualet Wang <mr.asianwang@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QStringList>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define DISPLAY_SERVICE "com.deepin.daemon.Display"
#define DISPLAY_PATH "/com/deepin/daemon/Display"
#define PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define DISPLAY_INTERFACE "com.deepin.daemon.Display"
#define MONITOR_INTERFACE "com.deepin.daemon.Display.Monitor"

class QX11Info;
class QString;
class QPoint;
class QRect;

namespace Utils {

bool menuItemCheckableFromId(QString id);

void grabKeyboard(xcb_window_t);
void grabPointer(xcb_window_t);

bool pointInRect(QPoint, QRect);
QRect currentMonitorRect(int, int);

}

#endif // UTILS_H
