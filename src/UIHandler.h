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

// UIHandler.h

#ifndef __UI_HANDLER_H__
#define __UI_HANDLER_H__

#include <gtk/gtk.h>

#include "UserSettings.h"

class CSmartEngine;

class CUIHandler
{
public:
    CUIHandler(CSmartEngine* pEngine);
    virtual ~CUIHandler();
    int Initialize();
    int CreateMainWnd();
    void DrawFrame(GdkPixbuf* frame);
    void UpdateOnDisconnected();
    void UpdateOnConnected();
    void UpdateOnNoNetwork();
    GtkWidget* GetMainWindow();
    void ShowMainWindow();
    void HideMainWindow();
    void SetMainWndPos(gint posX, gint posY);
    void OnMainWndMinimized(gboolean isMainWndMinimized);
    gboolean IsMainWndMinimized();

    void SetStatusMenu(GtkWidget* menu);
    GtkWidget* GetStatusMenu();
    GtkStatusIcon* GetStatusIcon();
    GdkPixbuf* GetLogoIcon();
    void ShowSettingsDlg(void);

    void UpdateStatusbarConnIcon(ConnectionType connType);
    void UpdateStatusbarConnLabel(const gchar* labelConnection);
    void UpdateStatusbarFps(const gchar* labelFps);
    void UpdateStatusbarResolution(int width, int height);
    void ShowDeviceErrorDlg();

    void Cleanup();

    static void Msg(const char *fmt, ...);

private:
    void LoadIcons();
    void DestroyIcons();

	// signal handlers:
	static void OnSettingsClicked(GtkToolButton *toolbutton, gint index);
	static void OnDisconnectClicked(GtkToolButton *toolbutton, gint index);
    static void OnRadiobuttonBluetooth(GtkToggleButton* btn, GtkWidget* portWidget);
    // Data:
    CSmartEngine* pSmartEngine;
    gboolean isMainWndMinimized;
    gint mainWndPosX;
    gint mainWndPosY;

    // Icons:
    GdkPixbuf* btStatusIcon;
    GdkPixbuf* inetStatusIcon;
    GdkPixbuf* connectedTrayIcon;
    GdkPixbuf* disconnectedTrayIcon;
    GdkPixbuf* logoIcon;

    // widgets:
    GtkWidget* mainWindow;
    GtkWidget* trayMenu;
    GtkWidget* toolbar;
    GtkWidget* miSettings;
    GtkToolItem* tbSettings;
    GtkToolItem* tbDisconnect;
    GtkWidget* image;
    GtkStatusIcon* trayIcon;
    GtkWidget* statusbar;
    GtkWidget *statusbarImageConnection;
    GtkWidget* statusbarLabelConnection;
    GtkWidget* statusbarLabelFps;
    GtkWidget* statusbarLabelResolution;
    gboolean main_wnd_minimized;

    // Window sizes:
    static const int MAIN_WND_WIDTH = 360;
    static const int MAIN_WND_HEIGHT = 372;

    // Tray icon:
    static const char* TRAY_TOOLTIP_CONNECTED;
    static const char* TRAY_TOOLTIP_DISCONNECTED;
    static const char* TRAY_TOOLTIP_NO_NETWORK;
    static const char* SMARTCAM_WND_TITLE;
    static const char* STATUS_MSG_DISCONNECTED;
    static const char* STATUS_MSG_CONNECTED;
    static const char* STATUS_LABEL_FPS;
    static const char* STATUS_LABEL_RESOLUTION;
};

#endif//__UI_HANDLER_H__
