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

// UserSettings.h

#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__

#define SMARTCAM_GCONF_ROOT "/apps/smartcam/"

typedef enum ConnectionType {
    CONN_BLUETOOTH = 0,
    CONN_INET = 1
} ConnectionType;

class CUserSettings
{
    friend class CSmartEngine;
public:
    CUserSettings();
    CUserSettings(const CUserSettings& settings);
    CUserSettings& operator=(const CUserSettings& settings);
    virtual ~CUserSettings();
    ConnectionType connectionType;
    int inetPort;

private:
    static CUserSettings LoadSettings();
    static void SaveSettings(CUserSettings settings);
    // Default settings:
    static const ConnectionType SMARTCAM_DEFAULT_CONNECTION_TYPE = CONN_BLUETOOTH;
    static const int SMARTCAM_DEFAULT_INET_PORT = 9361;
};
#endif//__USER_SETTINGS_H__
