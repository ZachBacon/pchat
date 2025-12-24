/* PChat
 * Copyright (C) 2025 Zach Bacon
 * Based on HexChat
 * Copyright (C) 2015 Arnav Singh.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* This file loads the WinRT notification DLL dynamically.
 * If the DLL is not available (e.g., on MinGW builds or if compilation failed),
 * it gracefully fails and the notification system won't work.
 * Consider using notification-windows-simple.c instead for MinGW builds. */

#include "../../common/config.h"
#include "../../common/pchat.h"
#include "../../common/plugin.h"
#include "../../common/util.h"

#include <gmodule.h>
#include <windows.h>

void (*winrt_notification_backend_show) (const char *title, const char *text) = NULL;
int (*winrt_notification_backend_init) (const char **error) = NULL;
void (*winrt_notification_backend_deinit) (void) = NULL;
int (*winrt_notification_backend_supported) (void) = NULL;

void
notification_backend_show (const char *title, const char *text)
{
	if (winrt_notification_backend_show == NULL)
	{
		return;
	}

	winrt_notification_backend_show (title, text);
}

int
notification_backend_init (const char **error)
{
	UINT original_error_mode;
	GModule *module;
	char *dll_path = NULL;

	/* Build the path to the DLL, supporting portable mode */
	if (portable_mode ())
	{
		char *path = g_win32_get_package_installation_directory_of_module (NULL);
		if (path)
		{
			dll_path = g_build_filename (path, "plugins", "pcnotifications-winrt.dll", NULL);
			g_free (path);
		}
		else
		{
			dll_path = g_strdup (".\\plugins\\pcnotifications-winrt.dll");
		}
	}
	else
	{
		dll_path = g_strdup (PCHATLIBDIR "\\plugins\\pcnotifications-winrt.dll");
	}

	/* Temporarily suppress the "DLL could not be loaded" dialog box before trying to load pcnotifications-winrt.dll */
	original_error_mode = GetErrorMode ();
	SetErrorMode(SEM_FAILCRITICALERRORS);
	module = module_load (dll_path);
	SetErrorMode (original_error_mode);

	g_free (dll_path);

	if (module == NULL)
	{
		*error = "pcnotifications-winrt not found. Using fallback notifications.";
		return 0;
	}

	g_module_symbol (module, "notification_backend_show", (gpointer *) &winrt_notification_backend_show);
	g_module_symbol (module, "notification_backend_init", (gpointer *) &winrt_notification_backend_init);
	g_module_symbol (module, "notification_backend_deinit", (gpointer *) &winrt_notification_backend_deinit);
	g_module_symbol (module, "notification_backend_supported", (gpointer *) &winrt_notification_backend_supported);

	return winrt_notification_backend_init (error);
}

void
notification_backend_deinit (void)
{
	if (winrt_notification_backend_deinit == NULL)
	{
		return;
	}

	winrt_notification_backend_deinit ();
}

int
notification_backend_supported (void)
{
	if (winrt_notification_backend_supported == NULL)
	{
		return 0;
	}

	return winrt_notification_backend_supported ();
}
