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

// UserSettings.cpp

#include <stdio.h>
#include <gconf/gconf-client.h>

#include "UserSettings.h"

// Constructor, loads with default user settings
CUserSettings::CUserSettings():
    connectionType(SMARTCAM_DEFAULT_CONNECTION_TYPE),
    inetPort(SMARTCAM_DEFAULT_INET_PORT)
{
}

CUserSettings::CUserSettings(const CUserSettings& settings):
    connectionType(settings.connectionType),
    inetPort(settings.inetPort)
{
}

CUserSettings& CUserSettings::operator=(const CUserSettings& settings)
{
    if(this != &settings)
    {
        connectionType = settings.connectionType;
        inetPort = settings.inetPort;
    }
    return *this;
}

CUserSettings::~CUserSettings()
{
}

CUserSettings CUserSettings::LoadSettings()
{
    CUserSettings regSettings; // default settings constructor
    GConfClient* gcClient = gconf_client_get_default();
    GConfValue* val = gconf_client_get_without_default(gcClient , SMARTCAM_GCONF_ROOT "connection_type", NULL);
    if(val != NULL)
    {
        // Check whether the value stored behind the key is an integer
        if(val->type == GCONF_VALUE_INT)
        {
            regSettings.connectionType = (ConnectionType)gconf_value_get_int(val);
        }
        gconf_value_free(val);
    }//if NULL val was not present in GConf db
    val = gconf_client_get_without_default(gcClient , SMARTCAM_GCONF_ROOT "inet_port", NULL);
    if(val != NULL)
    {
        // Check whether the value stored behind the key is an integer
        if(val->type == GCONF_VALUE_INT)
        {
            regSettings.inetPort = gconf_value_get_int(val);
        }
        gconf_value_free(val);
    }//if NULL val was not present in GConf db

    g_object_unref(gcClient);
    return regSettings;
}

void CUserSettings::SaveSettings(CUserSettings settings)
{
    GConfClient* gcClient = gconf_client_get_default();
    if(!gconf_client_set_int(gcClient , SMARTCAM_GCONF_ROOT "connection_type", settings.connectionType, NULL))
    {
        printf("smartcam: failed to set %s/connection_type to %d\n", SMARTCAM_GCONF_ROOT, settings.connectionType);
    }
    if(!gconf_client_set_int(gcClient , SMARTCAM_GCONF_ROOT "inet_port", settings.inetPort, NULL))
    {
        printf("smartcam: failed to set %s/inet_port to %d\n", SMARTCAM_GCONF_ROOT, settings.inetPort);
    }
    g_object_unref(gcClient);
}
