/*
 * Copyright (C) 2008 Ionut Dediu <deionut@yahoo.com>
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

#include "SmartEngine.h"

CSmartEngine* g_pEngine = NULL;

int main(int argc, char *argv[])
{
    int result = 0;

    GError* error = NULL;

    // init threads
    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);

    // Create the engine object
    g_pEngine = new CSmartEngine();

    result = g_pEngine->Initialize();
    if(result != 0)
    {
        g_pEngine->Cleanup(FALSE);
        delete g_pEngine;
        return -1;
    }

    result = g_pEngine->StartUI();
    if(result != 0)
    {
        g_pEngine->Cleanup(FALSE);
        delete g_pEngine;
        return -1;
    }

    result = g_pEngine->StartCommThread();
    if(result != 0)
    {
        g_pEngine->Cleanup(FALSE);
        delete g_pEngine;
        return -1;
    }

    gtk_main();
    gdk_threads_leave();

    delete g_pEngine;

    return 0;
}

