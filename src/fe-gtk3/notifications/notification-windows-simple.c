/* PChat
 * Copyright (C) 2025 Zach Bacon
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

/* MinGW-compatible Windows notification backend using Shell_NotifyIcon */

#include "../../common/pchat.h"
#include <windows.h>
#include <shellapi.h>
#include <glib.h>

/* We'll use the tray icon to show balloon notifications */
static NOTIFYICONDATAW nid = {0};
static gboolean initialized = FALSE;

void
notification_backend_show (const char *title, const char *text)
{
	wchar_t *wtitle, *wtext;
	
	if (!initialized)
		return;

	/* Convert UTF-8 to wide char */
	wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
	wtext = g_utf8_to_utf16 (text, -1, NULL, NULL, NULL);
	
	if (wtitle && wtext)
	{
		/* Set up balloon notification */
		nid.uFlags = NIF_INFO;
		nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
		
		/* Copy title and text (limited to NOTIFYICON sizes) */
		wcsncpy (nid.szInfoTitle, wtitle, sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1);
		nid.szInfoTitle[sizeof(nid.szInfoTitle)/sizeof(wchar_t) - 1] = 0;
		
		wcsncpy (nid.szInfo, wtext, sizeof(nid.szInfo)/sizeof(wchar_t) - 1);
		nid.szInfo[sizeof(nid.szInfo)/sizeof(wchar_t) - 1] = 0;
		
		/* Show the notification */
		Shell_NotifyIconW (NIM_MODIFY, &nid);
	}
	
	g_free (wtitle);
	g_free (wtext);
}

int
notification_backend_init (const char **error)
{
	/* We don't need to initialize anything special */
	/* The tray icon should already exist from plugin-tray.c */
	/* We'll just use it to show balloon notifications */
	
	initialized = TRUE;
	return 1;
}

void
notification_backend_deinit (void)
{
	initialized = FALSE;
}

int
notification_backend_supported (void)
{
	/* Windows balloon notifications are supported on Windows 2000+ */
	return 1;
}
