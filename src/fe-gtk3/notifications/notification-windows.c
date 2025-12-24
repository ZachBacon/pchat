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
 * If the DLL is not available, it falls back to Shell_NotifyIcon. */

#include "../../common/config.h"
#include "../../common/pchat.h"
#include "../../common/plugin.h"
#include "../../common/util.h"

#include <gmodule.h>
#include <windows.h>
#include <shellapi.h>

/* WinRT DLL function pointers */
void (*winrt_notification_backend_show) (const char *title, const char *text) = NULL;
int (*winrt_notification_backend_init) (const char **error) = NULL;
void (*winrt_notification_backend_deinit) (void) = NULL;
int (*winrt_notification_backend_supported) (void) = NULL;

/* Simple fallback using Shell_NotifyIcon */
static NOTIFYICONDATAW nid = {0};
static gboolean simple_backend_active = FALSE;

static void
simple_notification_show (const char *title, const char *text)
{
	wchar_t *wtitle, *wtext;
	
	if (!simple_backend_active)
		return;

	wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
	wtext = g_utf8_to_utf16 (text, -1, NULL, NULL, NULL);
	
	if (wtitle && wtext)
	{
		nid.uFlags = NIF_INFO;
		nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
		
		wcsncpy (nid.szInfoTitle, wtitle, sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1);
		nid.szInfoTitle[sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1] = 0;
		
		wcsncpy (nid.szInfo, wtext, sizeof(nid.szInfo)/sizeof(wchar_t) - 1);
		nid.szInfo[sizeof(nid.szInfo)/sizeof(wchar_t) - 1] = 0;
		
		Shell_NotifyIconW (NIM_MODIFY, &nid);
	}
	
	g_free (wtitle);
	g_free (wtext);
}

void
notification_backend_show (const char *title, const char *text)
{
	if (winrt_notification_backend_show != NULL)
	{
		winrt_notification_backend_show (title, text);
	}
	else if (simple_backend_active)
	{
		simple_notification_show (title, text);
	}
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
		/* WinRT DLL not available, use simple Shell_NotifyIcon fallback */
		simple_backend_active = TRUE;
		*error = NULL;
		return 1;
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
	if (winrt_notification_backend_deinit != NULL)
	{
		winrt_notification_backend_deinit ();
	}
	
	simple_backend_active = FALSE;
}

int
notification_backend_supported (void)
{
	if (winrt_notification_backend_supported != NULL)
	{
		return winrt_notification_backend_supported ();
	}
	
	/* Simple backend is always supported on Windows */
	return simple_backend_active ? 1 : 0;
}
