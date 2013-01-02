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

// UIHandler.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "UIHandler.h"
#include "smartcam.h"

#include "SmartEngine.h"

const char* CUIHandler::TRAY_TOOLTIP_CONNECTED = "SmartCam - Connected";
const char* CUIHandler::TRAY_TOOLTIP_DISCONNECTED = "SmartCam - Disconnected";
const char* CUIHandler::TRAY_TOOLTIP_NO_NETWORK = "SmartCam - No available network";
const char* CUIHandler::SMARTCAM_WND_TITLE = "SmartCam";
const char* CUIHandler::STATUS_MSG_DISCONNECTED = "Disconnected";
const char* CUIHandler::STATUS_MSG_CONNECTED = "Connected";
const char* CUIHandler::STATUS_LABEL_FPS = "FPS:";
const char* CUIHandler::STATUS_LABEL_RESOLUTION = "Resolution:";

static gint delete_event(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    gint main_wnd_pos_x = 0, main_wnd_pos_y = 0;
    gtk_window_get_position(GTK_WINDOW(g_pEngine->GetMainWindow()), &main_wnd_pos_x, &main_wnd_pos_y);
    g_pEngine->SetMainWndPos(main_wnd_pos_x, main_wnd_pos_y);
    gtk_widget_hide_all(widget);
    return TRUE;
}

static void destroy(GtkWidget* widget, gpointer data)
{
    g_pEngine->ExitApp(FALSE);
}

static void track_minimize(GtkWidget *widget, GdkEventWindowState *event, gpointer *statusbar)
{
    if(event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
    {
        g_pEngine->OnMainWndMinimized(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED);
    }
}

static void status_activate(GtkStatusIcon* stat_icon, gpointer user_data)
{
    gboolean main_wnd_visible = FALSE;
    if(g_pEngine->IsMainWndMinimized())
    {
        gtk_window_present(GTK_WINDOW(g_pEngine->GetMainWindow()));
        return;
    }
    g_object_get(G_OBJECT(g_pEngine->GetMainWindow()), "visible", &main_wnd_visible, NULL);
    if(main_wnd_visible)
    {
        if(!gtk_window_has_toplevel_focus(GTK_WINDOW(g_pEngine->GetMainWindow())))
        {
            g_pEngine->ShowMainWindow();
        }
        else
        {
            g_pEngine->HideMainWindow();
        }
    }
    else
    {
        g_pEngine->ShowMainWindow();
    }
}

void show_about_dialog(GtkWidget *widget, gpointer data)
{
    const gchar* authors[] =
    {
        "Ionut Dediu <deionut@yahoo.com>",
        "special thanks to Tomas Janousek <tomi@nomi.cz> \nfor YUYV support ",
        NULL
    };

    gtk_show_about_dialog(NULL,
                          "name","SmartCam",
                          "authors", authors,
                          "website", "http://sourceforge.net/projects/smartcam",
                          "copyright", "Copyright © 2008 Ionut Dediu",
                          "comments", "SmartCam - Turns a smartphone with bluetooth and camera into a handy webcam ready to use with your PC.",
                          "version", SMARTCAM_VERSION,
                          "logo-icon-name", "smartcam",
                          NULL);
}

static void status_popup(GtkStatusIcon *trayIcon, guint button,
                         guint activate_time, gpointer user_data)
{
    if(g_pEngine->GetStatusMenu() == NULL)
    {
        GtkWidget* menu_item_about, *menu_item_quit;

        g_pEngine->SetStatusMenu(gtk_menu_new());

        menu_item_about = gtk_image_menu_item_new_from_stock("gtk-about", NULL);
        gtk_container_add(GTK_CONTAINER(g_pEngine->GetStatusMenu()), menu_item_about);
        g_signal_connect(menu_item_about, "activate", G_CALLBACK(show_about_dialog), NULL);

        menu_item_quit = gtk_image_menu_item_new_from_stock("gtk-quit", NULL);
        gtk_container_add(GTK_CONTAINER(g_pEngine->GetStatusMenu()), menu_item_quit);
        g_signal_connect(G_OBJECT(menu_item_quit), "activate", G_CALLBACK(destroy), NULL);
    }

    gtk_widget_show_all(g_pEngine->GetStatusMenu());

    gtk_menu_popup(GTK_MENU(g_pEngine->GetStatusMenu()), NULL, NULL,
                   gtk_status_icon_position_menu, g_pEngine->GetStatusIcon(),
                   button, activate_time);

}

void CUIHandler::OnSettingsClicked(GtkToolButton *toolbutton, gint index)
{
    g_pEngine->ShowSettingsDlg();
}

void CUIHandler::OnDisconnectClicked(GtkToolButton *toolbutton, gint index)
{
    g_pEngine->Disconnect();
}

CUIHandler::CUIHandler(CSmartEngine* pEngine):
        pSmartEngine(pEngine),
        isMainWndMinimized(FALSE),
        mainWndPosX(0),
        mainWndPosY(0),
        btStatusIcon(NULL),
        inetStatusIcon(NULL),
        connectedTrayIcon(NULL),
        disconnectedTrayIcon(NULL),
        logoIcon(NULL),
        mainWindow(NULL),
        trayMenu(NULL),
        toolbar(NULL),
        miSettings(NULL),
        tbSettings(NULL),
        tbDisconnect(NULL),
        image(NULL),
        trayIcon(NULL),
        statusbar(NULL),
        statusbarImageConnection(NULL),
        statusbarLabelConnection(NULL),
        statusbarLabelFps(NULL),
        statusbarLabelResolution(NULL),
        main_wnd_minimized(FALSE)
{
}

CUIHandler::~CUIHandler()
{
    // No implementation required
}

void CUIHandler::LoadIcons()
{
    btStatusIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/bt.png", NULL);
    inetStatusIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/inet.png", NULL);
    connectedTrayIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/connected.png", NULL);
    connectedTrayIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/connected.png", NULL);
    disconnectedTrayIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/disconnected.png", NULL);
    logoIcon = gdk_pixbuf_new_from_file(PACKAGE_DATADIR "/icons/logo.png", NULL);
}

void CUIHandler::DestroyIcons()
{
    g_object_unref(btStatusIcon);
    g_object_unref(inetStatusIcon);
    g_object_unref(connectedTrayIcon);
    g_object_unref(disconnectedTrayIcon);
    g_object_unref(logoIcon);
}

int CUIHandler::Initialize()
{
    LoadIcons();
    return 0;
}

int CUIHandler::CreateMainWnd()
{
    GtkWidget* menubar;
    GtkWidget* filemenu, *helpmenu;
    GtkWidget* file, *quit;
    GtkWidget* help, *about;
    GtkWidget* vbox, *hbox;
    GtkWidget* align;
    GtkToolItem* toolitem;
    GtkWidget* frame;
    GtkWidget* status_menu;
    GtkWidget* status_menu_item;
    GtkWidget *hseparator;

    GtkWidget *statusbarHBox;
    GtkWidget *statusbarFrameConnection;
    GtkWidget *statusbarAlignmentConnection;
    GtkWidget *statusbarHBoxConnection;
    GtkWidget *statusbarFrameFps;
    GtkWidget *statusbarAlignmentFps;
    GtkWidget *statusbarFrameResolution;
    GtkWidget *statusbarAlignmentResolution;

    // set up tray icon
    trayIcon = gtk_status_icon_new_from_pixbuf(disconnectedTrayIcon);
    gtk_status_icon_set_tooltip(trayIcon, TRAY_TOOLTIP_DISCONNECTED);
    gtk_status_icon_set_visible(trayIcon, TRUE);
    g_signal_connect(G_OBJECT(trayIcon), "popup-menu", G_CALLBACK(status_popup), NULL);
    g_signal_connect(G_OBJECT(trayIcon), "activate", G_CALLBACK(status_activate), NULL);

    // set up main mainWindow
    mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(mainWindow), "delete_event", G_CALLBACK(delete_event), NULL);
    g_signal_connect(G_OBJECT(mainWindow), "destroy", G_CALLBACK(destroy), NULL);
    g_signal_connect(G_OBJECT(mainWindow), "window_state_event", G_CALLBACK(track_minimize), NULL);

    gtk_window_set_title(GTK_WINDOW(mainWindow), SMARTCAM_WND_TITLE);
    gtk_container_set_border_width(GTK_CONTAINER(mainWindow), 1);
    gtk_window_set_resizable(GTK_WINDOW(mainWindow), FALSE);
    gtk_widget_set_size_request(mainWindow, MAIN_WND_WIDTH, MAIN_WND_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(mainWindow), GTK_WIN_POS_NONE);
    gtk_window_set_gravity(GTK_WINDOW (mainWindow), GDK_GRAVITY_CENTER);

    gtk_window_set_default_icon(connectedTrayIcon);

    GdkPixbuf* icon;
    GList* icon_list = NULL;
    icon = gdk_pixbuf_new_from_file(DATADIR "/icons/hicolor/16x16/apps/smartcam.png", NULL);
    icon_list = g_list_append(icon_list, icon);
    icon = gdk_pixbuf_new_from_file(DATADIR "/icons/hicolor/22x22/apps/smartcam.png", NULL);
    icon_list = g_list_append(icon_list, icon);
    icon = gdk_pixbuf_new_from_file(DATADIR "/icons/hicolor/24x24/apps/smartcam.png", NULL);
    icon_list = g_list_append(icon_list, icon);
    icon = gdk_pixbuf_new_from_file(DATADIR "/icons/hicolor/32x32/apps/smartcam.png", NULL);
    icon_list = g_list_append(icon_list, icon);
    icon = gdk_pixbuf_new_from_file(DATADIR "/icons/hicolor/48x48/apps/smartcam.png", NULL);
    icon_list = g_list_append(icon_list, icon);

    gtk_window_set_default_icon_list(icon_list);

    g_list_foreach(icon_list, (GFunc)g_object_unref, NULL );
    g_list_free(icon_list);

    // create menubar
    menubar = gtk_menu_bar_new();

    filemenu = gtk_menu_new();
    file = gtk_menu_item_new_with_mnemonic("_File");
    miSettings = gtk_image_menu_item_new_with_label("Settings");
    gtk_image_menu_item_set_image(
        GTK_IMAGE_MENU_ITEM(miSettings),
        gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
    g_signal_connect(G_OBJECT(miSettings), "activate", G_CALLBACK(OnSettingsClicked), (gpointer) NULL);
    quit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(G_OBJECT(quit), "activate", G_CALLBACK(destroy), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), miSettings);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);

    helpmenu = gtk_menu_new();
    help = gtk_menu_item_new_with_mnemonic("_Help");
    about = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    g_signal_connect(about, "activate", G_CALLBACK(show_about_dialog), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), about);

    // create toolbar
    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
    g_object_set(G_OBJECT(toolbar), "can-focus", FALSE, NULL);

    tbSettings = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
    g_signal_connect(G_OBJECT(tbSettings), "clicked", G_CALLBACK(OnSettingsClicked), NULL);
    g_object_set(G_OBJECT(tbSettings), "tooltip-text", "Settings", NULL);
    g_object_set(G_OBJECT(tbSettings), "can-focus", FALSE, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tbSettings, 0);

    tbDisconnect = gtk_tool_button_new_from_stock(GTK_STOCK_DISCONNECT);
    g_signal_connect(G_OBJECT(tbDisconnect), "clicked", G_CALLBACK(OnDisconnectClicked), NULL);
    g_object_set(G_OBJECT(tbDisconnect), "tooltip-text", "Disconnect", NULL);
    g_object_set(G_OBJECT(tbDisconnect), "can-focus", FALSE, NULL);
    g_object_set(G_OBJECT(tbDisconnect), "sensitive", FALSE, NULL); // disable disconnect
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tbDisconnect, 1);

    // create preview frame
    vbox = gtk_vbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 1);
    gtk_container_add(GTK_CONTAINER(mainWindow), vbox);

    align = gtk_alignment_new(0.5, 0.5, 0, 0);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

    image = gtk_image_new_from_pixbuf(logoIcon);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), align, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(align), frame);
    gtk_container_add(GTK_CONTAINER(frame), image);

    // set up status bar
    hseparator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, FALSE, 0);

    statusbarHBox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), statusbarHBox, FALSE, FALSE, 0);

    statusbarFrameConnection = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (statusbarHBox), statusbarFrameConnection, TRUE, TRUE, 0);
    gtk_widget_set_size_request (statusbarFrameConnection, 55, 25);
    gtk_frame_set_label_align (GTK_FRAME (statusbarFrameConnection), 0, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (statusbarFrameConnection), GTK_SHADOW_IN);

    statusbarAlignmentConnection = gtk_alignment_new (0, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (statusbarFrameConnection), statusbarAlignmentConnection);
    gtk_alignment_set_padding (GTK_ALIGNMENT (statusbarAlignmentConnection), 0, 0, 3, 0);

    statusbarHBoxConnection = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (statusbarAlignmentConnection), statusbarHBoxConnection);

    if(pSmartEngine->GetSettings().connectionType == CONN_BLUETOOTH)
    {
        statusbarImageConnection = gtk_image_new_from_pixbuf(btStatusIcon);
    }
    else
    {
        statusbarImageConnection = gtk_image_new_from_pixbuf(inetStatusIcon);
    }
    gtk_box_pack_start (GTK_BOX (statusbarHBoxConnection), statusbarImageConnection, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (statusbarImageConnection), 0, 0.5);

    statusbarLabelConnection = gtk_label_new ("Disconnected");
    gtk_box_pack_start (GTK_BOX (statusbarHBoxConnection), statusbarLabelConnection, TRUE, TRUE, 0);
    gtk_misc_set_alignment (GTK_MISC (statusbarLabelConnection), 0, 0.5);

    statusbarFrameFps = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (statusbarHBox), statusbarFrameFps, TRUE, TRUE, 0);
    gtk_widget_set_size_request (statusbarFrameFps, 17, 25);
    gtk_frame_set_shadow_type (GTK_FRAME (statusbarFrameFps), GTK_SHADOW_IN);

    statusbarAlignmentFps = gtk_alignment_new (0, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (statusbarFrameFps), statusbarAlignmentFps);
    gtk_alignment_set_padding (GTK_ALIGNMENT (statusbarAlignmentFps), 0, 0, 3, 0);

    statusbarLabelFps = gtk_label_new (CUIHandler::STATUS_LABEL_FPS);
    gtk_container_add (GTK_CONTAINER (statusbarAlignmentFps), statusbarLabelFps);
    gtk_misc_set_alignment (GTK_MISC (statusbarLabelFps), 0, 0.5);

    statusbarFrameResolution = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (statusbarHBox), statusbarFrameResolution, TRUE, TRUE, 0);
    gtk_widget_set_size_request (statusbarFrameResolution, 92, 25);
    gtk_frame_set_shadow_type (GTK_FRAME (statusbarFrameResolution), GTK_SHADOW_IN);

    statusbarAlignmentResolution = gtk_alignment_new (0, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (statusbarFrameResolution), statusbarAlignmentResolution);
    gtk_alignment_set_padding (GTK_ALIGNMENT (statusbarAlignmentResolution), 0, 0, 3, 0);

    statusbarLabelResolution = gtk_label_new (CUIHandler::STATUS_LABEL_RESOLUTION);
    gtk_container_add (GTK_CONTAINER (statusbarAlignmentResolution), statusbarLabelResolution);
    gtk_misc_set_alignment (GTK_MISC (statusbarLabelResolution), 0, 0.5);

    gtk_widget_show_all(mainWindow);

    return 0;
}

void CUIHandler::OnRadiobuttonBluetooth(GtkToggleButton* btn, GtkWidget* portWidget)
{
    if(gtk_toggle_button_get_active(btn))
    {
        g_object_set(G_OBJECT(portWidget), "sensitive", FALSE, NULL);
    }
    else
    {
        g_object_set(G_OBJECT(portWidget), "sensitive", TRUE, NULL);
    }
}

void CUIHandler::ShowSettingsDlg(void)
{
    GtkWidget* settingsDlg;
    GtkWidget* dialog_vbox1;
    GtkWidget* frame4;
    GtkWidget* alignment4;
    GtkWidget* vbox2;
    GtkWidget* radiobuttonBt;
    GtkWidget* hbox2;
    GtkWidget* radiobuttonInet;
    GtkWidget* label5;
    GtkWidget* inetPort;
    GtkWidget* label4;
    GtkWidget* dialog_action_area1;

    CUserSettings crtSettings = pSmartEngine->GetSettings();

    settingsDlg = gtk_dialog_new_with_buttons("SmartCam Preferences",
                        GTK_WINDOW(pSmartEngine->GetMainWindow()),
                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    gtk_window_set_type_hint(GTK_WINDOW (settingsDlg), GDK_WINDOW_TYPE_HINT_DIALOG);

    dialog_vbox1 = GTK_DIALOG (settingsDlg)->vbox;
    gtk_widget_show (dialog_vbox1);

    frame4 = gtk_frame_new (NULL);
    gtk_widget_show (frame4);
    gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame4, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_IN);

    alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_widget_show (alignment4);
    gtk_container_add (GTK_CONTAINER (frame4), alignment4);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment4), 10, 10, 12, 12);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (alignment4), vbox2);

    radiobuttonBt = gtk_radio_button_new_with_mnemonic(NULL, "Bluetooth");
    gtk_box_pack_start(GTK_BOX (vbox2), radiobuttonBt, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox2);
    gtk_box_pack_start(GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

    radiobuttonInet = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(radiobuttonBt), "TCP/IP (WiFi)");
    gtk_box_pack_start(GTK_BOX (hbox2), radiobuttonInet, FALSE, FALSE, 0);

    label5 = gtk_label_new("    Port: ");
    gtk_box_pack_start(GTK_BOX(hbox2), label5, FALSE, FALSE, 0);

    inetPort = gtk_spin_button_new_with_range(1025, 65536, 1);
    gtk_box_pack_start(GTK_BOX(hbox2), inetPort, TRUE, TRUE, 0);

    label4 = gtk_label_new("Connection");
    gtk_frame_set_label_widget(GTK_FRAME(frame4), label4);
    gtk_label_set_use_markup(GTK_LABEL(label4), TRUE);

    dialog_action_area1 = GTK_DIALOG(settingsDlg)->action_area;
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);

    g_signal_connect(G_OBJECT(radiobuttonBt), "toggled", G_CALLBACK(OnRadiobuttonBluetooth), inetPort);
    
    if(crtSettings.connectionType == CONN_BLUETOOTH)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonBt), TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonInet), FALSE);
        g_object_set(G_OBJECT(inetPort), "sensitive", FALSE, NULL);
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonBt), FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonInet), TRUE);
        g_object_set(G_OBJECT(inetPort), "sensitive", TRUE, NULL);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(inetPort), crtSettings.inetPort);
    gtk_widget_show_all(settingsDlg);

    if(gtk_dialog_run(GTK_DIALOG(settingsDlg)) == GTK_RESPONSE_OK)
    {
        CUserSettings newSettings = crtSettings;
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobuttonBt)))
        {
            newSettings.connectionType = CONN_BLUETOOTH;
        }
        else
        {
            newSettings.connectionType = CONN_INET;
            newSettings.inetPort = gtk_spin_button_get_value(GTK_SPIN_BUTTON(inetPort));
        }
        pSmartEngine->SaveSettings(newSettings);
    }
    gtk_widget_destroy(settingsDlg);
}

void CUIHandler::DrawFrame(GdkPixbuf* frame)
{
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), frame);
    gtk_widget_queue_draw(image);
}

void CUIHandler::Cleanup()
{
    DestroyIcons();
}

void CUIHandler::Msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void CUIHandler::UpdateOnDisconnected()
{
    gdk_threads_enter();
    printf("smartcam: disconnected\n");
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), logoIcon);
    gtk_widget_queue_draw(image);
    gtk_status_icon_set_from_pixbuf(trayIcon, disconnectedTrayIcon);
    gtk_status_icon_set_tooltip(trayIcon, TRAY_TOOLTIP_DISCONNECTED);
    g_object_set(G_OBJECT(tbSettings), "sensitive", TRUE, NULL);    // enable settings
    g_object_set(G_OBJECT(miSettings), "sensitive", TRUE, NULL);
    g_object_set(G_OBJECT(tbDisconnect), "sensitive", FALSE, NULL); // disable disconnect
    gtk_label_set_text(GTK_LABEL(statusbarLabelConnection), STATUS_MSG_DISCONNECTED);
    gtk_label_set_text(GTK_LABEL(statusbarLabelFps), STATUS_LABEL_FPS);
    gtk_label_set_text(GTK_LABEL(statusbarLabelResolution), STATUS_LABEL_RESOLUTION);
    gdk_threads_leave();
}

void CUIHandler::UpdateOnConnected()
{
    gdk_threads_enter();
    gtk_status_icon_set_from_pixbuf(trayIcon, connectedTrayIcon);
    gtk_status_icon_set_tooltip(trayIcon, TRAY_TOOLTIP_CONNECTED);
    g_object_set(G_OBJECT(tbSettings), "sensitive", FALSE, NULL);    // disable settings
    g_object_set(G_OBJECT(miSettings), "sensitive", FALSE, NULL);
    g_object_set(G_OBJECT(tbDisconnect), "sensitive", TRUE, NULL);   // enable disconnect
    gtk_label_set_text(GTK_LABEL(statusbarLabelConnection), STATUS_MSG_CONNECTED);
    gdk_threads_leave();
}

void CUIHandler::UpdateOnNoNetwork()
{
}

GtkWidget* CUIHandler::GetMainWindow()
{
    return mainWindow;
}

void CUIHandler::ShowMainWindow()
{
    gtk_widget_set_uposition(mainWindow, mainWndPosX, mainWndPosY);
    gtk_widget_show_all(mainWindow);
    gtk_window_present(GTK_WINDOW(mainWindow));
}

void CUIHandler::HideMainWindow()
{
    gtk_window_get_position(GTK_WINDOW(mainWindow), &mainWndPosX, &mainWndPosY);
    gtk_widget_hide_all(mainWindow);
}

void CUIHandler::SetMainWndPos(gint posX, gint posY)
{
    mainWndPosX = posX;
    mainWndPosY = posY;
}

void CUIHandler::OnMainWndMinimized(gboolean isMainWndMinimized)
{
    gtk_window_get_position(GTK_WINDOW(mainWindow), &mainWndPosX, &mainWndPosY);
    this->isMainWndMinimized = isMainWndMinimized;
}

gboolean CUIHandler::IsMainWndMinimized()
{
    return isMainWndMinimized;
}

void CUIHandler::SetStatusMenu(GtkWidget* menu)
{
    trayMenu = menu;
}

GtkWidget* CUIHandler::GetStatusMenu()
{
    return trayMenu;
}

GtkStatusIcon* CUIHandler::GetStatusIcon()
{
    return trayIcon;
}

GdkPixbuf* CUIHandler::GetLogoIcon()
{
    return logoIcon;
}

void CUIHandler::UpdateStatusbarConnIcon(ConnectionType connType)
{
    if(connType == CONN_BLUETOOTH)
    {
        gtk_image_set_from_pixbuf(GTK_IMAGE(statusbarImageConnection), btStatusIcon);
    }
    else
    {
        gtk_image_set_from_pixbuf(GTK_IMAGE(statusbarImageConnection), inetStatusIcon);
    }
}

void CUIHandler::UpdateStatusbarConnLabel(const gchar* labelConnection)
{
    gtk_label_set_text(GTK_LABEL(statusbarLabelConnection), labelConnection);
}

void CUIHandler::UpdateStatusbarFps(const gchar* labelFps)
{
    gtk_label_set_text(GTK_LABEL(statusbarLabelFps), labelFps);
}

void CUIHandler::UpdateStatusbarResolution(int width, int height)
{
    if(width == -1 || height == -1)
    {
        gtk_label_set_text(GTK_LABEL(statusbarLabelResolution), STATUS_LABEL_RESOLUTION);
    }
    else
    {
        char resolution_str[30];
        memset(resolution_str, 0, 30);
        sprintf(resolution_str, "Resolution: %d x %d", width, height);
        gtk_label_set_text(GTK_LABEL(statusbarLabelResolution), resolution_str);
    }
}

void CUIHandler::ShowDeviceErrorDlg()
{
    GtkWidget* label = NULL;
    GtkHBox* dialog_hbox = NULL;
    GtkImage* dialog_icon = NULL;

    GtkDialog* dialog = GTK_DIALOG(
        gtk_dialog_new_with_buttons(
            "SmartCam Error",          // title
            GTK_WINDOW(mainWindow),   // parent
            GTK_DIALOG_MODAL,          // options
            // list of button labels and responses
            "_Ok", GTK_RESPONSE_CLOSE,
            NULL)
        );
    // fill dialog window: create HBox packed with icon and Text
    dialog_hbox = (GtkHBox*) g_object_new(GTK_TYPE_HBOX, NULL);
    dialog_icon = (GtkImage*) g_object_new(GTK_TYPE_IMAGE, "stock", GTK_STOCK_DIALOG_ERROR, NULL);
    gtk_box_pack_start(GTK_BOX(dialog_hbox), GTK_WIDGET(dialog_icon), FALSE, FALSE, 0);

    label = gtk_label_new("\n   Could not find smartcam device file.\n   Please load (insmod) the driver and chmod 666 /dev/videoX.\n");
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(dialog_hbox), label, TRUE, TRUE, 0);
    // pack HBox into dialog
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), GTK_WIDGET(dialog_hbox));
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_widget_show_all(GTK_WIDGET(dialog));
}

