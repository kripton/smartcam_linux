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

// SmartEngine.cpp

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <dbus/dbus-glib-lowlevel.h>    // dbus_connection_setup_with_g_main
#include <gdk/gdkx.h>

#include "SmartEngine.h"
#include "CommHandler.h"
#include "UIHandler.h"
#include "JpegHandler.h"
#include "smartcam.h"

#define SMARTCAM_DRIVER_NAME "smartcam"

static void term_handler(int signo)
{
    g_pEngine->ExitApp(TRUE);
}

CSmartEngine::CSmartEngine():
        commThread(NULL),
        dbusConnection(NULL),
        crtWidth(-1),
        crtHeight(-1),
        lastSampleTimeMillis(0),
        crtSampleFrames(0),
        deviceFd(-1),
        isAlive(0),
        pCommHandler(NULL),
        pJpegHandler(NULL),
        pUIHandler(NULL),
        crtSettings()
{
}

CSmartEngine::~CSmartEngine()
{
    if(pCommHandler != NULL)
    {
        delete pCommHandler;
        pCommHandler = NULL;
    }
    if(pJpegHandler != NULL)
    {
        delete pJpegHandler;
        pJpegHandler = NULL;
    }
    if(pUIHandler != NULL)
    {
        delete pUIHandler;
        pUIHandler = NULL;
    }
}

DBusHandlerResult CSmartEngine::dbus_msg_handler(
        DBusConnection *connection, DBusMessage *message, void *user_data)
{
    gboolean handled = FALSE;
    if(dbus_message_is_method_call(message, SMARTCAM_DBUS_INTERFACE, SMARTCAM_DBUS_BRING_TO_FRONT_METHOD_NAME))
    {
        g_pEngine->BringToFrontDBusCB(message, connection);
        handled = TRUE;
    }
    return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

void CSmartEngine::BringToFrontDBusCB(DBusMessage *message, DBusConnection *connection)
{
    DBusMessage* reply = NULL;
    guint32 startup_timestamp = gdk_x11_get_server_time(GTK_WIDGET(g_pEngine->GetMainWindow())->window);
    gdk_x11_window_set_user_time(GTK_WIDGET(g_pEngine->GetMainWindow())->window, startup_timestamp);
    //g_pEngine->ShowMainWindow();
    gtk_widget_show_all(g_pEngine->GetMainWindow());
    gtk_window_present(GTK_WINDOW(g_pEngine->GetMainWindow()));
    reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
}

int CSmartEngine::Initialize()
{
    int result = 0;
    DBusError dberr;
    DBusMessage *dbmsg;
    dbus_error_init(&dberr);
    dbusConnection = dbus_bus_get(DBUS_BUS_SESSION, &dberr);
    if(dbus_error_is_set(&dberr))
    {
        printf("smartcam: getting session bus failed: %s\n", dberr.message);
        dbus_error_free(&dberr);
        return -1;
    }

    result = dbus_bus_request_name(dbusConnection, SMARTCAM_DBUS_SERVICE, 0, &dberr);
    if(result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER && result != DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER)
    {
        printf("smartcam: another instance is already running, exiting ...\n");
        if(dbus_error_is_set(&dberr))
        {
            dbus_error_free(&dberr);
        }
        dbmsg = dbus_message_new_method_call(SMARTCAM_DBUS_SERVICE,
                                             SMARTCAM_DBUS_PATH,
                                             SMARTCAM_DBUS_INTERFACE,
                                             SMARTCAM_DBUS_BRING_TO_FRONT_METHOD_NAME);
        if(dbmsg == NULL)
        {
             printf ("smartcam: Couldn’t create a DBusMessage");
             return -1;
        }
        dbus_connection_send(dbusConnection, dbmsg, NULL);
        dbus_connection_flush(dbusConnection);
        dbus_message_unref(dbmsg);
        dbmsg = NULL;
        gdk_notify_startup_complete();
        return -1;
    }

    // Connect D-Bus to the mainloop
    dbus_connection_setup_with_g_main(dbusConnection, NULL);
    if(!dbus_connection_add_filter(dbusConnection, dbus_msg_handler, NULL, NULL))
    {
        printf("smartcam: failed to add D-Bus filter\n");
        return -1;
    }

    printf("smartcam: registered DBUS service \"%s\"\n", SMARTCAM_DBUS_SERVICE);
    // set up signal handlers
    if(signal(SIGTERM, term_handler) == SIG_ERR)
    {
        printf("smartcam: can not handle SIGTERM\n");
        return -1;
    }

    if(signal(SIGINT, term_handler) == SIG_ERR)
    {
        printf("smartcam: can not handle SIGINT\n");
        return -1;
    }

    pUIHandler = new CUIHandler(this);
    result = pUIHandler->Initialize();
    if (result != 0)
        return result;

    pCommHandler = new CCommHandler(this);
    result = pCommHandler->Initialize();
    if (result != 0)
        return result;

    pJpegHandler = new CJpegHandler();

    if(OpenSmartCamDevice() != 0)
    {
        pUIHandler->ShowDeviceErrorDlg();
    }
    
    crtSettings = CUserSettings::LoadSettings();

    // put logo image in the driver
    WriteDeviceFrame((const char*) gdk_pixbuf_get_pixels(pUIHandler->GetLogoIcon()), SMARTCAM_FRAME_SIZE);

    return 0;
}

void CSmartEngine::Cleanup(gboolean fromSignal)
{
    StopCommThread(fromSignal);
    // close DBUS
    if(dbusConnection != NULL)
    {
        dbus_connection_unref(dbusConnection);
        dbusConnection = NULL;
    }
    // put logo image in the driver
    if(pUIHandler != NULL && pUIHandler->GetLogoIcon() != NULL)
    {
        WriteDeviceFrame((const char*) gdk_pixbuf_get_pixels(pUIHandler->GetLogoIcon()), SMARTCAM_FRAME_SIZE);
    }

    // close smartcam device file
    if(deviceFd != -1)
        close(deviceFd);

    if(pCommHandler != NULL)
        pCommHandler->Cleanup();
    if(pUIHandler != NULL)
        pUIHandler->Cleanup();
}

int CSmartEngine::StartUI()
{
    return pUIHandler->CreateMainWnd();
}

void* CSmartEngine::CommThreadProc(void *args)
{
    int errCode = 0;
    int frameSize = 0;
    int crtBytesRcvd = 0;
    int totalBytesRcvd = 0;

    GError* error = NULL;

    g_pEngine->StartServer();

ACCEPT_CLIENT:
    while(1)
    {
        errCode = g_pEngine->AcceptClient();
        if(errCode == ACCEPT_OK)
        {
            break;
        }
        else if(errCode == ACCEPT_ERROR)
        {
            return NULL;
        }
        else if(errCode == ACCEPT_RETRY)
        {
            if(g_pEngine->isAlive)
            {
                usleep(300000);
            }
            else
            {
                return NULL;
            }
        }
    }

    while(g_pEngine->isAlive)
    {
        errCode = g_pEngine->RcvPacket();
        if (errCode == 0) // SUCCESS
        {
            g_pEngine->ProcessPacket();
        }
        else             // ERROR
        {
            if(g_pEngine->isAlive)
            {
                goto ACCEPT_CLIENT;
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}

int CSmartEngine::StartCommThread()
{
    // Create the comm thread
    isAlive = TRUE;
    GError* error = NULL;
    commThread = g_thread_create(CommThreadProc, NULL, TRUE, &error);
    if(commThread == NULL)
    {
        g_printerr("Failed to create comm thread: %s\n", error->message);
        g_error_free(error);
        return -1;
    }
    printf("smartcam: started comm thread\n");
    return 0;
}

void CSmartEngine::StopCommThread(gboolean fromSignal)
{
    isAlive = FALSE;
    if(!fromSignal)
        gdk_threads_leave();
    if(commThread)
        g_thread_join(commThread);
    if(!fromSignal)
        gdk_threads_enter();

    pCommHandler->StopServer();
    printf("smartcam: stopped comm thread\n");
    commThread = NULL;
}

int CSmartEngine::Disconnect()
{
    return pCommHandler->Disconnect();
}

gboolean CSmartEngine::IsConnected()
{
    return pCommHandler->IsConnected();
}

int CSmartEngine::xioctl(int fd, int request, void *arg)
{
    int r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int CSmartEngine::OpenSmartCamDevice()
{
    int crt_video_dev = 0;
    char dev_name[12];

    for(crt_video_dev = 0; crt_video_dev < 10; crt_video_dev++)
    {
        struct stat st;
        struct v4l2_capability v4l2cap;

        deviceFd = -1;

        sprintf(dev_name, "%s%d", "/dev/video", crt_video_dev);
        if(-1 == stat(dev_name, &st))
        {
            printf("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
            continue;
        }

        if(!S_ISCHR(st.st_mode))
        {
            printf("%s is no device\n", dev_name);
            continue;
        }

        deviceFd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

        if(-1 == deviceFd)
        {
            printf("Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
            continue;
        }
        if(-1 == xioctl(deviceFd, VIDIOC_QUERYCAP, &v4l2cap))
        {
            if(EINVAL == errno)
            {
                printf("%s is no V4L2 device\n", dev_name);
            }
            close(deviceFd);
            continue;
        }
        // the current video device is not the smartcam device file
        if(strncmp((const char*) v4l2cap.driver, SMARTCAM_DRIVER_NAME, 8))
        {
            close(deviceFd);
            continue;
        }
        // found the smartcam device
        else
        {
            printf("Found smartcam device file: %s\n", dev_name);
            return 0;
        }
    }
    return -1;
}

int CSmartEngine::StartServer()
{
    int result = 0;
    if (crtSettings.connectionType == CONN_INET)
        result = pCommHandler->StartInetServer(crtSettings.inetPort);
    else if (crtSettings.connectionType == CONN_BLUETOOTH)
        result = pCommHandler->StartBtServer();
    return result;
}

AcceptResultCode CSmartEngine::AcceptClient()
{
    AcceptResultCode result = ACCEPT_OK;
    if(crtSettings.connectionType == CONN_INET)
    {
        result = pCommHandler->AcceptInetClient();
    }
    else if(crtSettings.connectionType == CONN_BLUETOOTH)
    {
        result = pCommHandler->AcceptBtClient();
    }
    return result;
}

int CSmartEngine::RcvPacket()
{
    return pCommHandler->RcvPacket();
}

void CSmartEngine::ProcessPacket()
{
    if(pCommHandler->GetRcvPacketType() == PACKET_JPEG_HEDAER)
    {
        pJpegHandler->decodeHeader(pCommHandler->GetRcvPacket(), pCommHandler->GetRcvPacketLen());
    }
    else if(pCommHandler->GetRcvPacketType() == PACKET_JPEG_DATA)
    {
        int w = 0, h = 0;
        GdkPixbuf* pixbuf = NULL, * scaledPixbuf = NULL;
        unsigned char* driverBufferRgb24 = NULL;
        unsigned char* rgb24 = pJpegHandler->decodeRGB24(pCommHandler->GetRcvPacket(), pCommHandler->GetRcvPacketLen(), w, h);
        if(rgb24 == NULL)
        {
            return; // error, maybe just disconnected...
        }
        gdk_threads_enter();
        pixbuf = gdk_pixbuf_new_from_data(rgb24, GDK_COLORSPACE_RGB, FALSE, 8, w, h, w * 3, NULL, NULL);
        if(w != SMARTCAM_FRAME_WIDTH || h != SMARTCAM_FRAME_HEIGHT)
        {
            scaledPixbuf = gdk_pixbuf_scale_simple(
                                pixbuf, SMARTCAM_FRAME_WIDTH, SMARTCAM_FRAME_HEIGHT, GDK_INTERP_BILINEAR);
            g_object_unref(pixbuf);
            pixbuf = NULL;
            driverBufferRgb24 = gdk_pixbuf_get_pixels(scaledPixbuf);
        }
        else // do not scale, use original buffer/pixbuf
        {
            scaledPixbuf = pixbuf;
            driverBufferRgb24 = rgb24;
        }
        gdk_threads_leave();
        // write the frame in the driverb
        WriteDeviceFrame((const char*)driverBufferRgb24, SMARTCAM_FRAME_SIZE);
        // draw the frame
        gdk_threads_enter();
        pUIHandler->DrawFrame(scaledPixbuf);
        gdk_threads_leave();
        g_object_unref(scaledPixbuf);
        scaledPixbuf = NULL;
        SampleFPS();

        // Update resolution status bar message
        if(crtWidth != w || crtHeight != h)
        {
            crtWidth = w;
            crtHeight = h;
            gdk_threads_enter();
            pUIHandler->UpdateStatusbarResolution(crtWidth, crtHeight);
            gdk_threads_leave();
        }
    }
}

void CSmartEngine::WriteDeviceFrame(const char* frameData, int frameLength)
{
    if(deviceFd == -1)
    {
        return;
    }
    int result = 0;
    int size = frameLength;
    while(size > 0)
    {
        result = write(deviceFd, frameData + (frameLength - size), size);
        if(result == -1 && errno != EAGAIN)
        {
            printf("smartcam: error writing device frame: %s\n", strerror(errno));
            return;
        }
        else
        {
            size -= result;
        }
    }
}

void CSmartEngine::SampleFPS()
{
    struct timeval now = {0};
    if(gettimeofday(&now, NULL))
    {
        return;
    }
    unsigned long nowMillis = now.tv_sec * 1000 + now.tv_usec/1000;
    if(lastSampleTimeMillis == 0)
    {
        lastSampleTimeMillis = nowMillis;
        return;
    }
    unsigned long elapsedMillis = nowMillis - lastSampleTimeMillis;
    if(elapsedMillis >= 1000)
    {
        float fps = ((float)crtSampleFrames * 1000)/elapsedMillis;
        char fps_str[30];
        memset(fps_str, 0, 30);
        sprintf(fps_str, "FPS: %.2f", fps);
        gdk_threads_enter();
        pUIHandler->UpdateStatusbarFps(fps_str);
        gdk_threads_leave();
        lastSampleTimeMillis = nowMillis;
        crtSampleFrames = 0;
    }
    else
    {
        ++crtSampleFrames;
    }
}

void CSmartEngine::OnConnected()
{
    crtSampleFrames = 0;
    lastSampleTimeMillis = 0;
    pUIHandler->UpdateOnConnected();
}

void CSmartEngine::OnDisconnected()
{
    crtWidth = -1;
    crtHeight = -1;
    crtSampleFrames = 0;
    lastSampleTimeMillis = 0;
    WriteDeviceFrame((const char*) gdk_pixbuf_get_pixels(pUIHandler->GetLogoIcon()), SMARTCAM_FRAME_SIZE);
    pUIHandler->UpdateOnDisconnected();
}

GtkWidget* CSmartEngine::GetMainWindow()
{
    return pUIHandler->GetMainWindow();
}

void CSmartEngine::ShowMainWindow()
{
    pUIHandler->ShowMainWindow();
}

void CSmartEngine::HideMainWindow()
{
    pUIHandler->HideMainWindow();
}

void CSmartEngine::SetMainWndPos(gint posX, gint posY)
{
    pUIHandler->SetMainWndPos(posX, posY);
}

void CSmartEngine::OnMainWndMinimized(gboolean isMainWndMinimized)
{
    pUIHandler->OnMainWndMinimized(isMainWndMinimized);
}

gboolean CSmartEngine::IsMainWndMinimized()
{
    return pUIHandler->IsMainWndMinimized();
}

void CSmartEngine::SetStatusMenu(GtkWidget* menu)
{
    pUIHandler->SetStatusMenu(menu);
}

GtkWidget* CSmartEngine::GetStatusMenu()
{
    return pUIHandler->GetStatusMenu();
}

GtkStatusIcon* CSmartEngine::GetStatusIcon()
{
    return pUIHandler->GetStatusIcon();
}

void CSmartEngine::ExitApp(gboolean fromSignal)
{
    printf("smartcam: exit app\n");
    Cleanup(fromSignal);
    gtk_main_quit();
}

void CSmartEngine::ShowSettingsDlg(void)
{
    pUIHandler->ShowSettingsDlg();
}

CUserSettings CSmartEngine::GetSettings()
{
    return crtSettings;
}

void CSmartEngine::SaveSettings(CUserSettings settings)
{
    if((crtSettings.connectionType != settings.connectionType) ||
       (crtSettings.inetPort != settings.inetPort))
    {
        CUserSettings::SaveSettings(settings);
        CUserSettings oldSettings = crtSettings;
        crtSettings = settings;
        if((oldSettings.connectionType != settings.connectionType) ||
           (oldSettings.inetPort != settings.inetPort))
        {
            pUIHandler->UpdateStatusbarConnIcon(settings.connectionType);
            StopCommThread(FALSE);
            StartCommThread();
        }
    }
}
