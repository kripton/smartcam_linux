/*
 * Copyright (C) 2009 Ionut Dediu <deionut@yahoo.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// SmartEngine.h

#ifndef __SMART_ENGINE_H__
#define __SMART_ENGINE_H__

#include <gtk/gtk.h>
#include <dbus/dbus.h>

#include "CommHandler.h"
#include "UserSettings.h"

// SmartCam DBus service
#define SMARTCAM_DBUS_SERVICE                               "org.gnome.smartcam"
// SmartCam DBus interface
#define SMARTCAM_DBUS_INTERFACE                             "org.gnome.smartcam"
// SmartCam DBus path
#define SMARTCAM_DBUS_PATH                                  "/org/gnome/smartcam"
// SmartCam DBus bring to front method
#define SMARTCAM_DBUS_BRING_TO_FRONT_METHOD_NAME            "bring_to_front"

class CUIHandler;
class CJpegHandler;

class CSmartEngine
{
public:
    CSmartEngine();
    virtual ~CSmartEngine();
    int Initialize();
    void Cleanup(gboolean fromSignal);
    int StartUI();
    int StartCommThread();
    void StopCommThread(gboolean fromSignal);
    int Disconnect();
    void OnConnected();
    void OnDisconnected();
    GtkWidget* GetMainWindow();
    void ShowMainWindow();
    void HideMainWindow();
    void SetMainWndPos(gint posX, gint posY);
    void OnMainWndMinimized(gboolean isMainWndMinimized);
    gboolean IsMainWndMinimized();
    void SetStatusMenu(GtkWidget* menu);
    GtkWidget* GetStatusMenu();
    GtkStatusIcon* GetStatusIcon();
    gboolean IsConnected();
    void ShowSettingsDlg(void);
    CUserSettings GetSettings();
    void SaveSettings(CUserSettings settings);
    void ExitApp(gboolean fromSignal);

private:
    // Methods:
    int OpenSmartCamDevice();
    int StartServer();
    AcceptResultCode AcceptClient();
    int RcvPacket();
    void ProcessPacket();
    void WriteDeviceFrame(const char* frame_data, int frame_length);
    void SampleFPS();
    void BringToFrontDBusCB(DBusMessage *message, DBusConnection *connection);
    // Static methods:
    static int xioctl(int fd, int request, void *arg);
    static DBusHandlerResult dbus_msg_handler(DBusConnection *connection, DBusMessage *message, void *user_data);
    // Comm thread procedure:
    static void* CommThreadProc(void* args);

    // Data:
    GThread* commThread;
    DBusConnection* dbusConnection;
    int crtWidth;
    int crtHeight;
    unsigned long lastSampleTimeMillis;
    int crtSampleFrames;
    int deviceFd;
    // Comm thread
    gboolean isAlive;
    CCommHandler* pCommHandler;
    CJpegHandler* pJpegHandler;
    CUIHandler* pUIHandler;
    CUserSettings crtSettings;

    static const int SMARTCAM_FRAME_WIDTH = 320;
    static const int SMARTCAM_FRAME_HEIGHT = 240;
    static const int SMARTCAM_FRAME_SIZE = SMARTCAM_FRAME_WIDTH * SMARTCAM_FRAME_HEIGHT * 3;
};
#endif//__SMART_ENGINE_H__
