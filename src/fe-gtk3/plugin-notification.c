/* PChat
 * Copyright (C) 2025 Zach Bacon
 * Based on HexChat
 * Copyright (C) 2015 Patrick Griffis.
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

#include "../common/pchat.h"
#include "../common/pchat-plugin.h"
#include "../common/pchatc.h"
#include "../common/inbound.h"
#include "../common/text.h"
#include "../common/util.h"
#include "notifications/notification-backend.h"
#include "plugin-notification.h"
#include <glib.h>
#include <string.h>

static pchat_plugin *ph;

static gboolean
should_alert (void)
{
	const char *status;
	
	/* Check if we should omit alerts when focused */
	if (prefs.pchat_gui_focus_omitalerts)
	{
		status = pchat_get_info (ph, "win_status");
		if (status && !g_strcmp0 (status, "active"))
			return FALSE;
	}

	/* Check if away and should omit alerts when away */
	if (prefs.pchat_away_omit_alerts)
	{
		if (pchat_get_info (ph, "away"))
			return FALSE;
	}

	/* Check tray quiet mode */
	if (prefs.pchat_gui_tray_quiet)
	{
		if (prefs.pchat_gui_tray)
		{
			status = pchat_get_info (ph, "win_status");
			if (status && g_strcmp0 (status, "hidden") != 0)
				return FALSE;
		}
	}

	return TRUE;
}

static gboolean
is_ignored (char *nick)
{
	if (prefs.pchat_irc_no_hilight[0])
	{
		return alert_match_word (nick, prefs.pchat_irc_no_hilight);
	}
	return FALSE;
}

static void
show_notification (const char *title, const char *text)
{
	char *stripped_title, *stripped_text;

	/* Strip all colors and formatting */
	stripped_title = strip_color ((char*)title, -1, STRIP_ALL);
	stripped_text = strip_color ((char*)text, -1, STRIP_ALL);
	
	notification_backend_show (stripped_title, stripped_text);

	g_free (stripped_title);
	g_free (stripped_text);
}

static void
show_notificationf (const char *text, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	show_notification (buf, text);
	g_free (buf);
}

static int
incoming_hilight_cb (char *word[], void *userdata)
{
	if (prefs.pchat_input_balloon_hilight && should_alert() && !is_ignored(word[1]))
	{
		show_notificationf (word[2], _("Highlighted message from: %s (%s)"), 
		                    word[1], pchat_get_info (ph, "channel"));
	}
	return PCHAT_EAT_NONE;
}

static int
incoming_message_cb (char *word[], void *userdata)
{
	if (prefs.pchat_input_balloon_chans && should_alert())
	{
		show_notificationf (word[2], _("Channel message from: %s (%s)"), 
		                    word[1], pchat_get_info (ph, "channel"));
	}
	return PCHAT_EAT_NONE;
}

static int
incoming_priv_cb (char *word[], void *userdata)
{
	if (prefs.pchat_input_balloon_priv && should_alert() && !is_ignored(word[1]))
	{
		const char *network = pchat_get_info (ph, "network");
		if (network)
			show_notificationf (word[2], _("Private message from: %s (%s)"), word[1], network);
		else
			show_notificationf (word[2], _("Private message from: %s"), word[1]);
	}
	return PCHAT_EAT_NONE;
}

static int
tray_cmd_cb (char *word[], char *word_eol[], void *userdata)
{
	if (word[2] && !g_ascii_strcasecmp (word[2], "test"))
	{
		show_notification (_("PChat Notification Test"), _("This is a test notification"));
		return PCHAT_EAT_ALL;
	}

	return PCHAT_EAT_NONE;
}

int
notification_plugin_init (pchat_plugin *plugin_handle, char **plugin_name,
                          char **plugin_desc, char **plugin_version, char *arg)
{
	const char *error;

	/* Save plugin handle for use with pchat_* functions */
	ph = plugin_handle;

	*plugin_name = "";
	*plugin_desc = "";
	*plugin_version = "";

	if (!notification_backend_init (&error))
	{
		/* Silently fail - notifications just won't work */
		return 0;
	}

	if (!notification_backend_supported ())
	{
		/* Backend loaded but not supported on this system */
		notification_backend_deinit ();
		return 0;
	}

	pchat_hook_print (ph, "Channel Msg Hilight", PCHAT_PRI_NORM, incoming_hilight_cb, NULL);
	pchat_hook_print (ph, "Channel Action Hilight", PCHAT_PRI_NORM, incoming_hilight_cb, NULL);

	pchat_hook_print (ph, "Channel Message", PCHAT_PRI_NORM, incoming_message_cb, NULL);
	pchat_hook_print (ph, "Channel Action", PCHAT_PRI_NORM, incoming_message_cb, NULL);
	pchat_hook_print (ph, "Channel Msg", PCHAT_PRI_NORM, incoming_message_cb, NULL);

	pchat_hook_print (ph, "Private Message", PCHAT_PRI_NORM, incoming_priv_cb, NULL);
	pchat_hook_print (ph, "Private Message to Dialog", PCHAT_PRI_NORM, incoming_priv_cb, NULL);
	pchat_hook_print (ph, "Private Action", PCHAT_PRI_NORM, incoming_priv_cb, NULL);
	pchat_hook_print (ph, "Private Action to Dialog", PCHAT_PRI_NORM, incoming_priv_cb, NULL);

	pchat_hook_command (ph, "TRAY", PCHAT_PRI_NORM, tray_cmd_cb, 
	                    _("Usage: TRAY TEST - Test notification system"), NULL);

	return 1;
}

void
notification_plugin_deinit (void)
{
	notification_backend_deinit ();
}
