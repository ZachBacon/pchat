/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xchat.h"
#include "cfgfiles.h"
#include "util.h"
#include "fe.h"
#include "text.h"
#include "xchatc.h"
#include "typedef.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define XCHAT_DIR "xchat"
#endif

#define DEF_FONT "Monospace 9"
#define DEF_FONT_ALTER "Arial Unicode MS,Lucida Sans Unicode,MS Gothic,Unifont"

const char * const languages[LANGUAGES_LENGTH] = {
	"af", "sq", "am", "ast", "az", "eu", "be", "bg", "ca", "zh_CN",      /*  0 ..  9 */
	"zh_TW", "cs", "da", "nl", "en_GB", "en", "et", "fi", "fr", "gl",    /* 10 .. 19 */
	"de", "el", "gu", "hi", "hu", "id", "it", "ja_JP", "kn", "rw",       /* 20 .. 29 */
	"ko", "lv", "lt", "mk", "ml", "ms", "nb", "no", "pl", "pt",          /* 30 .. 39 */
	"pt_BR", "pa", "ru", "sr", "sk", "sl", "es", "sv", "th", "tr",       /* 40 .. 49 */
	"uk", "vi", "wa"                                                     /* 50 .. */
};

void
list_addentry (GSList ** list, char *cmd, char *name)
{
	struct popup *pop;
	size_t name_len;
	size_t cmd_len = 1;

	/* remove <2.8.0 stuff */
	if (!strcmp (cmd, "away") && !strcmp (name, "BACK"))
		return;

	if (cmd)
		cmd_len = strlen (cmd) + 1;
	name_len = strlen (name) + 1;

	pop = g_malloc (sizeof (struct popup) + cmd_len + name_len);
	pop->name = (char *) pop + sizeof (struct popup);
	pop->cmd = pop->name + name_len;

	memcpy (pop->name, name, name_len);
	if (cmd)
		memcpy (pop->cmd, cmd, cmd_len);
	else
		pop->cmd[0] = 0;

	*list = g_slist_append (*list, pop);
}

/* read it in from a buffer to our linked list */

static void
list_load_from_data (GSList ** list, char *ibuf, int size)
{
	char cmd[384];
	char name[128];
	char *buf;
	int pnt = 0;

	cmd[0] = 0;
	name[0] = 0;

	while (buf_get_line (ibuf, &buf, &pnt, size))
	{
		if (*buf != '#')
		{
			if (!g_ascii_strncasecmp (buf, "NAME ", 5))
			{
				safe_strcpy (name, buf + 5, sizeof (name));
			}
			else if (!g_ascii_strncasecmp (buf, "CMD ", 4))
			{
				safe_strcpy (cmd, buf + 4, sizeof (cmd));
				if (*name)
				{
					list_addentry (list, cmd, name);
					cmd[0] = 0;
					name[0] = 0;
				}
			}
		}
	}
}

void
list_loadconf (char *file, GSList ** list, char *defaultconf)
{
	char *filebuf;
	char *ibuf;
	int fd;
	struct stat st;

	filebuf = g_build_filename (get_xdir (), file, NULL);
	fd = g_open (filebuf, O_RDONLY | OFLAGS, 0);
	g_free (filebuf);

	if (fd == -1)
	{
		if (defaultconf)
			list_load_from_data (list, defaultconf, strlen (defaultconf));
		return;
	}
	if (fstat (fd, &st) != 0)
	{
		perror ("fstat");
		abort ();
	}

	ibuf = g_malloc (st.st_size);
	read (fd, ibuf, st.st_size);
	close (fd);

	list_load_from_data (list, ibuf, st.st_size);

	g_free (ibuf);
}

void
list_free (GSList ** list)
{
	void *data;
	while (*list)
	{
		data = (void *) (*list)->data;
		g_free (data);
		*list = g_slist_remove (*list, data);
	}
}

int
list_delentry (GSList ** list, char *name)
{
	struct popup *pop;
	GSList *alist = *list;

	while (alist)
	{
		pop = (struct popup *) alist->data;
		if (!g_ascii_strcasecmp (name, pop->name))
		{
			*list = g_slist_remove (*list, pop);
			g_free (pop);
			return 1;
		}
		alist = alist->next;
	}
	return 0;
}

char *
cfg_get_str (char *cfg, const char *var, char *dest, int dest_len)
{
	char buffer[128];	/* should be plenty for a variable name */

	sprintf (buffer, "%s ", var);	/* add one space, this way it works against var - var2 checks too */

	while (1)
	{
		if (!g_ascii_strncasecmp (buffer, cfg, strlen (var) + 1))
		{
			char *value, t;
			cfg += strlen (var);
			while (*cfg == ' ')
				cfg++;
			if (*cfg == '=')
				cfg++;
			while (*cfg == ' ')
				cfg++;
			/*while (*cfg == ' ' || *cfg == '=')
			   cfg++; */
			value = cfg;
			while (*cfg != 0 && *cfg != '\n')
				cfg++;
			t = *cfg;
			*cfg = 0;
			safe_strcpy (dest, value, dest_len);
			*cfg = t;
			return cfg;
		}
		while (*cfg != 0 && *cfg != '\n')
			cfg++;
		if (*cfg == 0)
			return NULL;
		cfg++;
		if (*cfg == 0)
			return NULL;
	}
}

static int
cfg_put_str (int fh, char *var, char *value)
{
	char buf[512];
	int len;

	g_snprintf (buf, sizeof buf, "%s = %s\n", var, value);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_put_color (int fh, guint16 r, guint16 g, guint16 b, char *var)
{
	char buf[400];
	int len;

	g_snprintf (buf, sizeof buf, "%s = %04"G_GUINT16_FORMAT" %04"G_GUINT16_FORMAT" %04"G_GUINT16_FORMAT"\n", var, r, g, b);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_put_int (int fh, int value, char *var)
{
	char buf[400];
	int len;

	if (value == -1)
		value = 1;

	g_snprintf (buf, sizeof buf, "%s = %d\n", var, value);
	len = strlen (buf);
	return (write (fh, buf, len) == len);
}

int
cfg_get_color (char *cfg, char *var, guint16 *r, guint16 *g, guint16 *b)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
		return 0;

	sscanf (str, "%04"G_GUINT16_FORMAT" %04"G_GUINT16_FORMAT" %04"G_GUINT16_FORMAT, r, g, b);
	return 1;
}

int
cfg_get_int_with_result (char *cfg, char *var, int *result)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
	{
		*result = 0;
		return 0;
	}

	*result = 1;
	return atoi (str);
}

int
cfg_get_int (char *cfg, char *var)
{
	char str[128];

	if (!cfg_get_str (cfg, var, str, sizeof (str)))
		return 0;

	return atoi (str);
}

char *xdir = NULL;	/* utf-8 encoding */

#ifdef _WIN32

#include <windows.h>

static gboolean
get_reg_str (const char *sub, const char *name, char *out, DWORD len)
{
	HKEY hKey;
	DWORD t;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, sub, 0, KEY_READ, &hKey) ==
			ERROR_SUCCESS)
	{
		if (RegQueryValueEx (hKey, name, NULL, &t, out, &len) != ERROR_SUCCESS ||
			 t != REG_SZ)
		{
			RegCloseKey (hKey);
			return FALSE;
		}
		out[len-1] = 0;
		RegCloseKey (hKey);
		return TRUE;
	}

	return FALSE;
}
#endif

char *
get_xdir (void)
{
	if (!xdir)
	{
#ifdef _WIN32
		char out[256];

		if (portable_mode () || !get_reg_str ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "AppData", out, sizeof (out)))
		{
			char *path;
			path = g_win32_get_package_installation_directory_of_module (NULL);
			if (path)
			{
				xdir = g_build_filename (path, "config", NULL);
				g_free (path);
			}
			else
				xdir = g_strdup (".\\config");
		}
		else
		{
			xdir = g_build_filename (out, "PChat", NULL);
		}
#else
		xdir = g_build_filename (g_get_user_config_dir (), XCHAT_DIR, NULL);
#endif
	}

	return xdir;
}

int
check_config_dir (void)
{
	return g_access (get_xdir (), F_OK);
}

static char *
default_file (void)
{
	static char *dfile = NULL;

	if (!dfile)
	{
		dfile = g_build_filename (get_xdir (), "xchat.conf", NULL);
	}
	return dfile;
}

/* Keep these sorted!! */

const struct prefs vars[] =
{
	{"away_auto_unmark", P_OFFINT (pchat_away_auto_unmark), TYPE_BOOL},
	{"away_omit_alerts", P_OFFINT (pchat_away_omit_alerts), TYPE_BOOL},
	{"away_reason", P_OFFSET (pchat_away_reason), TYPE_STR},
	{"away_show_once", P_OFFINT (pchat_away_show_once), TYPE_BOOL},
	{"away_size_max", P_OFFINT (pchat_away_size_max), TYPE_INT},
	{"away_timeout", P_OFFINT (pchat_away_timeout), TYPE_INT},
	{"away_track", P_OFFINT (pchat_away_track), TYPE_BOOL},

	{"completion_amount", P_OFFINT (pchat_completion_amount), TYPE_INT},
	{"completion_auto", P_OFFINT (pchat_completion_auto), TYPE_BOOL},
	{"completion_sort", P_OFFINT (pchat_completion_sort), TYPE_INT},
	{"completion_suffix", P_OFFSET (pchat_completion_suffix), TYPE_STR},

	{"dcc_auto_chat", P_OFFINT (pchat_dcc_auto_chat), TYPE_BOOL},
	{"dcc_auto_recv", P_OFFINT (pchat_dcc_auto_recv), TYPE_INT},
	{"dcc_auto_resume", P_OFFINT (pchat_dcc_auto_resume), TYPE_BOOL},
	{"dcc_blocksize", P_OFFINT (pchat_dcc_blocksize), TYPE_INT},
	{"dcc_completed_dir", P_OFFSET (pchat_dcc_completed_dir), TYPE_STR},
	{"dcc_dir", P_OFFSET (pchat_dcc_dir), TYPE_STR},
#ifndef _WIN32
	{"dcc_fast_send", P_OFFINT (pchat_dcc_fast_send), TYPE_BOOL},
#endif
	{"dcc_global_max_get_cps", P_OFFINT (pchat_dcc_global_max_get_cps), TYPE_INT},
	{"dcc_global_max_send_cps", P_OFFINT (pchat_dcc_global_max_send_cps), TYPE_INT},
	{"dcc_ip", P_OFFSET (pchat_dcc_ip), TYPE_STR},
	{"dcc_ip_from_server", P_OFFINT (pchat_dcc_ip_from_server), TYPE_BOOL},
	{"dcc_max_get_cps", P_OFFINT (pchat_dcc_max_get_cps), TYPE_INT},
	{"dcc_max_send_cps", P_OFFINT (pchat_dcc_max_send_cps), TYPE_INT},
	{"dcc_permissions", P_OFFINT (pchat_dcc_permissions), TYPE_INT},
	{"dcc_port_first", P_OFFINT (pchat_dcc_port_first), TYPE_INT},
	{"dcc_port_last", P_OFFINT (pchat_dcc_port_last), TYPE_INT},
	{"dcc_remove", P_OFFINT (pchat_dcc_remove), TYPE_BOOL},
	{"dcc_save_nick", P_OFFINT (pchat_dcc_save_nick), TYPE_BOOL},
	{"dcc_send_fillspaces", P_OFFINT (pchat_dcc_send_fillspaces), TYPE_BOOL},
	{"dcc_stall_timeout", P_OFFINT (pchat_dcc_stall_timeout), TYPE_INT},
	{"dcc_timeout", P_OFFINT (pchat_dcc_timeout), TYPE_INT},

	{"flood_ctcp_num", P_OFFINT (pchat_flood_ctcp_num), TYPE_INT},
	{"flood_ctcp_time", P_OFFINT (pchat_flood_ctcp_time), TYPE_INT},
	{"flood_msg_num", P_OFFINT (pchat_flood_msg_num), TYPE_INT},
	{"flood_msg_time", P_OFFINT (pchat_flood_msg_time), TYPE_INT},

	{"gui_autoopen_chat", P_OFFINT (pchat_gui_autoopen_chat), TYPE_BOOL},
	{"gui_autoopen_dialog", P_OFFINT (pchat_gui_autoopen_dialog), TYPE_BOOL},
	{"gui_autoopen_recv", P_OFFINT (pchat_gui_autoopen_recv), TYPE_BOOL},
	{"gui_autoopen_send", P_OFFINT (pchat_gui_autoopen_send), TYPE_BOOL},
	{"gui_chanlist_maxusers", P_OFFINT (pchat_gui_chanlist_maxusers), TYPE_INT},
	{"gui_chanlist_minusers", P_OFFINT (pchat_gui_chanlist_minusers), TYPE_INT},
	{"gui_compact", P_OFFINT (pchat_gui_compact), TYPE_BOOL},
	{"gui_dialog_height", P_OFFINT (pchat_gui_dialog_height), TYPE_INT},
	{"gui_dialog_left", P_OFFINT (pchat_gui_dialog_left), TYPE_INT},
	{"gui_dialog_top", P_OFFINT (pchat_gui_dialog_top), TYPE_INT},
	{"gui_dialog_width", P_OFFINT (pchat_gui_dialog_width), TYPE_INT},
	{"gui_focus_omitalerts", P_OFFINT (pchat_gui_focus_omitalerts), TYPE_BOOL},
	{"gui_hide_menu", P_OFFINT (pchat_gui_hide_menu), TYPE_BOOL},
	{"gui_input_attr", P_OFFINT (pchat_gui_input_attr), TYPE_BOOL},
	{"gui_input_icon", P_OFFINT (pchat_gui_input_icon), TYPE_BOOL},
	{"gui_input_nick", P_OFFINT (pchat_gui_input_nick), TYPE_BOOL},
	{"gui_input_spell", P_OFFINT (pchat_gui_input_spell), TYPE_BOOL},
	{"gui_input_style", P_OFFINT (pchat_gui_input_style), TYPE_BOOL},
	{"gui_join_dialog", P_OFFINT (pchat_gui_join_dialog), TYPE_BOOL},
	{"gui_lagometer", P_OFFINT (pchat_gui_lagometer), TYPE_INT},
	{"gui_lang", P_OFFINT (pchat_gui_lang), TYPE_INT},
	{"gui_mode_buttons", P_OFFINT (pchat_gui_mode_buttons), TYPE_BOOL},
	{"gui_pane_divider_position", P_OFFINT (pchat_gui_pane_divider_position), TYPE_INT},
	{"gui_pane_left_size", P_OFFINT (pchat_gui_pane_left_size), TYPE_INT},
	{"gui_pane_right_size", P_OFFINT (pchat_gui_pane_right_size), TYPE_INT},
	{"gui_pane_right_size_min", P_OFFINT (pchat_gui_pane_right_size_min), TYPE_INT},
	{"gui_quit_dialog", P_OFFINT (pchat_gui_quit_dialog), TYPE_BOOL},
	{"gui_search_pos", P_OFFINT (pchat_gui_search_pos), TYPE_INT},
	/* {"gui_single", P_OFFINT (pchat_gui_single), TYPE_BOOL}, */
	{"gui_slist_fav", P_OFFINT (pchat_gui_slist_fav), TYPE_BOOL},
	{"gui_slist_select", P_OFFINT (pchat_gui_slist_select), TYPE_INT},
	{"gui_slist_skip", P_OFFINT (pchat_gui_slist_skip), TYPE_BOOL},
	{"gui_tab_chans", P_OFFINT (pchat_gui_tab_chans), TYPE_BOOL},
	{"gui_tab_dialogs", P_OFFINT (pchat_gui_tab_dialogs), TYPE_BOOL},
	{"gui_tab_dots", P_OFFINT (pchat_gui_tab_dots), TYPE_BOOL},
	{"gui_tab_icons", P_OFFINT (pchat_gui_tab_icons), TYPE_BOOL},
	{"gui_tab_layout", P_OFFINT (pchat_gui_tab_layout), TYPE_INT},
	{"gui_tab_newtofront", P_OFFINT (pchat_gui_tab_newtofront), TYPE_INT},
	{"gui_tab_pos", P_OFFINT (pchat_gui_tab_pos), TYPE_INT},
	{"gui_tab_scrollchans", P_OFFINT (pchat_gui_tab_scrollchans), TYPE_BOOL},
	{"gui_tab_server", P_OFFINT (pchat_gui_tab_server), TYPE_BOOL},
	{"gui_tab_small", P_OFFINT (pchat_gui_tab_small), TYPE_INT},
	{"gui_tab_sort", P_OFFINT (pchat_gui_tab_sort), TYPE_BOOL},
	{"gui_tab_trunc", P_OFFINT (pchat_gui_tab_trunc), TYPE_INT},
	{"gui_tab_utils", P_OFFINT (pchat_gui_tab_utils), TYPE_BOOL},
	{"gui_throttlemeter", P_OFFINT (pchat_gui_throttlemeter), TYPE_INT},
	{"gui_topicbar", P_OFFINT (pchat_gui_topicbar), TYPE_BOOL},
	{"gui_transparency", P_OFFINT (pchat_gui_transparency), TYPE_INT},
	{"gui_tray", P_OFFINT (pchat_gui_tray), TYPE_BOOL},
	{"gui_tray_away", P_OFFINT (pchat_gui_tray_away), TYPE_BOOL},
	{"gui_tray_blink", P_OFFINT (pchat_gui_tray_blink), TYPE_BOOL},
	{"gui_tray_close", P_OFFINT (pchat_gui_tray_close), TYPE_BOOL},
	{"gui_tray_minimize", P_OFFINT (pchat_gui_tray_minimize), TYPE_BOOL},
	{"gui_tray_quiet", P_OFFINT (pchat_gui_tray_quiet), TYPE_BOOL},
	{"gui_ulist_buttons", P_OFFINT (pchat_gui_ulist_buttons), TYPE_BOOL},
	{"gui_ulist_color", P_OFFINT (pchat_gui_ulist_color), TYPE_BOOL},
	{"gui_ulist_count", P_OFFINT (pchat_gui_ulist_count), TYPE_BOOL},
	{"gui_ulist_doubleclick", P_OFFSET (pchat_gui_ulist_doubleclick), TYPE_STR},
	{"gui_ulist_hide", P_OFFINT (pchat_gui_ulist_hide), TYPE_BOOL},
	{"gui_ulist_icons", P_OFFINT (pchat_gui_ulist_icons), TYPE_BOOL},
	{"gui_ulist_pos", P_OFFINT (pchat_gui_ulist_pos), TYPE_INT},
	{"gui_ulist_resizable", P_OFFINT (pchat_gui_ulist_resizable), TYPE_BOOL},
	{"gui_ulist_show_hosts", P_OFFINT(pchat_gui_ulist_show_hosts), TYPE_BOOL},
	{"gui_ulist_sort", P_OFFINT (pchat_gui_ulist_sort), TYPE_INT},
	{"gui_ulist_style", P_OFFINT (pchat_gui_ulist_style), TYPE_BOOL},
	{"gui_url_mod", P_OFFINT (pchat_gui_url_mod), TYPE_INT},
	{"gui_usermenu", P_OFFINT (pchat_gui_usermenu), TYPE_BOOL},
	{"gui_win_height", P_OFFINT (pchat_gui_win_height), TYPE_INT},
	{"gui_win_fullscreen", P_OFFINT (pchat_gui_win_fullscreen), TYPE_INT},
	{"gui_win_left", P_OFFINT (pchat_gui_win_left), TYPE_INT},
	{"gui_win_modes", P_OFFINT (pchat_gui_win_modes), TYPE_BOOL},
	{"gui_win_save", P_OFFINT (pchat_gui_win_save), TYPE_BOOL},
	{"gui_win_state", P_OFFINT (pchat_gui_win_state), TYPE_INT},
	{"gui_win_swap", P_OFFINT (pchat_gui_win_swap), TYPE_BOOL},
	{"gui_win_top", P_OFFINT (pchat_gui_win_top), TYPE_INT},
	{"gui_win_ucount", P_OFFINT (pchat_gui_win_ucount), TYPE_BOOL},
	{"gui_win_width", P_OFFINT (pchat_gui_win_width), TYPE_INT},

	{"identd", P_OFFINT (pchat_identd), TYPE_BOOL},

	{"input_balloon_chans", P_OFFINT (pchat_input_balloon_chans), TYPE_BOOL},
	{"input_balloon_hilight", P_OFFINT (pchat_input_balloon_hilight), TYPE_BOOL},
	{"input_balloon_priv", P_OFFINT (pchat_input_balloon_priv), TYPE_BOOL},
	{"input_balloon_time", P_OFFINT (pchat_input_balloon_time), TYPE_INT},
	{"input_beep_chans", P_OFFINT (pchat_input_beep_chans), TYPE_BOOL},
	{"input_beep_hilight", P_OFFINT (pchat_input_beep_hilight), TYPE_BOOL},
	{"input_beep_priv", P_OFFINT (pchat_input_beep_priv), TYPE_BOOL},
	{"input_command_char", P_OFFSET (pchat_input_command_char), TYPE_STR},
	{"input_filter_beep", P_OFFINT (pchat_input_filter_beep), TYPE_BOOL},
	{"input_flash_chans", P_OFFINT (pchat_input_flash_chans), TYPE_BOOL},
	{"input_flash_hilight", P_OFFINT (pchat_input_flash_hilight), TYPE_BOOL},
	{"input_flash_priv", P_OFFINT (pchat_input_flash_priv), TYPE_BOOL},
	{"input_perc_ascii", P_OFFINT (pchat_input_perc_ascii), TYPE_BOOL},
	{"input_perc_color", P_OFFINT (pchat_input_perc_color), TYPE_BOOL},
	{"input_tray_chans", P_OFFINT (pchat_input_tray_chans), TYPE_BOOL},
	{"input_tray_hilight", P_OFFINT (pchat_input_tray_hilight), TYPE_BOOL},
	{"input_tray_priv", P_OFFINT (pchat_input_tray_priv), TYPE_BOOL},

	{"irc_auto_rejoin", P_OFFINT (pchat_irc_auto_rejoin), TYPE_BOOL},
	{"irc_ban_type", P_OFFINT (pchat_irc_ban_type), TYPE_INT},
	{"irc_cap_server_time", P_OFFINT (pchat_irc_cap_server_time), TYPE_BOOL},
	{"irc_conf_mode", P_OFFINT (pchat_irc_conf_mode), TYPE_BOOL},
	{"irc_extra_hilight", P_OFFSET (pchat_irc_extra_hilight), TYPE_STR},
	{"irc_hide_version", P_OFFINT (pchat_irc_hide_version), TYPE_BOOL},
	{"irc_hidehost", P_OFFINT (pchat_irc_hidehost), TYPE_BOOL},
	{"irc_id_ntext", P_OFFSET (pchat_irc_id_ntext), TYPE_STR},
	{"irc_id_ytext", P_OFFSET (pchat_irc_id_ytext), TYPE_STR},
	{"irc_invisible", P_OFFINT (pchat_irc_invisible), TYPE_BOOL},
	{"irc_join_delay", P_OFFINT (pchat_irc_join_delay), TYPE_INT},
	{"irc_logging", P_OFFINT (pchat_irc_logging), TYPE_BOOL},
	{"irc_logmask", P_OFFSET (pchat_irc_logmask), TYPE_STR},
	{"irc_nick1", P_OFFSET (pchat_irc_nick1), TYPE_STR},
	{"irc_nick2", P_OFFSET (pchat_irc_nick2), TYPE_STR},
	{"irc_nick3", P_OFFSET (pchat_irc_nick3), TYPE_STR},
	{"irc_nick_hilight", P_OFFSET (pchat_irc_nick_hilight), TYPE_STR},
	{"irc_no_hilight", P_OFFSET (pchat_irc_no_hilight), TYPE_STR},
	{"irc_notice_pos", P_OFFINT (pchat_irc_notice_pos), TYPE_INT},
	{"irc_part_reason", P_OFFSET (pchat_irc_part_reason), TYPE_STR},
	{"irc_quit_reason", P_OFFSET (pchat_irc_quit_reason), TYPE_STR},
	{"irc_raw_modes", P_OFFINT (pchat_irc_raw_modes), TYPE_BOOL},
	{"irc_real_name", P_OFFSET (pchat_irc_real_name), TYPE_STR},
	{"irc_servernotice", P_OFFINT (pchat_irc_servernotice), TYPE_BOOL},
	{"irc_skip_motd", P_OFFINT (pchat_irc_skip_motd), TYPE_BOOL},
	{"irc_user_name", P_OFFSET (pchat_irc_user_name), TYPE_STR},
	{"irc_wallops", P_OFFINT (pchat_irc_wallops), TYPE_BOOL},
	{"irc_who_join", P_OFFINT (pchat_irc_who_join), TYPE_BOOL},
	{"irc_whois_front", P_OFFINT (pchat_irc_whois_front), TYPE_BOOL},

	{"net_auto_reconnect", P_OFFINT (pchat_net_auto_reconnect), TYPE_BOOL},
#ifndef _WIN32	/* FIXME fix reconnect crashes and remove this ifdef! */
	{"net_auto_reconnectonfail", P_OFFINT (pchat_net_auto_reconnectonfail), TYPE_BOOL},
#endif
	{"net_bind_host", P_OFFSET (pchat_net_bind_host), TYPE_STR},
	{"net_ping_timeout", P_OFFINT (pchat_net_ping_timeout), TYPE_INT},
	{"net_proxy_auth", P_OFFINT (pchat_net_proxy_auth), TYPE_BOOL},
	{"net_proxy_host", P_OFFSET (pchat_net_proxy_host), TYPE_STR},
	{"net_proxy_pass", P_OFFSET (pchat_net_proxy_pass), TYPE_STR},
	{"net_proxy_port", P_OFFINT (pchat_net_proxy_port), TYPE_INT},
	{"net_proxy_type", P_OFFINT (pchat_net_proxy_type), TYPE_INT},
	{"net_proxy_use", P_OFFINT (pchat_net_proxy_use), TYPE_INT},
	{"net_proxy_user", P_OFFSET (pchat_net_proxy_user), TYPE_STR},
	{"net_reconnect_delay", P_OFFINT (pchat_net_reconnect_delay), TYPE_INT},
	{"net_throttle", P_OFFINT (pchat_net_throttle), TYPE_BOOL},

	{"notify_timeout", P_OFFINT (pchat_notify_timeout), TYPE_INT},
	{"notify_whois_online", P_OFFINT (pchat_notify_whois_online), TYPE_BOOL},

	{"perl_warnings", P_OFFINT (pchat_perl_warnings), TYPE_BOOL},

	{"stamp_log", P_OFFINT (pchat_stamp_log), TYPE_BOOL},
	{"stamp_log_format", P_OFFSET (pchat_stamp_log_format), TYPE_STR},
	{"stamp_text", P_OFFINT (pchat_stamp_text), TYPE_BOOL},
	{"stamp_text_format", P_OFFSET (pchat_stamp_text_format), TYPE_STR},

	{"text_autocopy_color", P_OFFINT (pchat_text_autocopy_color), TYPE_BOOL},
	{"text_autocopy_stamp", P_OFFINT (pchat_text_autocopy_stamp), TYPE_BOOL},
	{"text_autocopy_text", P_OFFINT (pchat_text_autocopy_text), TYPE_BOOL},
	{"text_background", P_OFFSET (pchat_text_background), TYPE_STR},
	{"text_color_nicks", P_OFFINT (pchat_text_color_nicks), TYPE_BOOL},
	{"text_font", P_OFFSET (pchat_text_font), TYPE_STR},
	{"text_font_main", P_OFFSET (pchat_text_font_main), TYPE_STR},
	{"text_font_alternative", P_OFFSET (pchat_text_font_alternative), TYPE_STR},
	{"text_indent", P_OFFINT (pchat_text_indent), TYPE_BOOL},
	{"text_max_indent", P_OFFINT (pchat_text_max_indent), TYPE_INT},
	{"text_max_lines", P_OFFINT (pchat_text_max_lines), TYPE_INT},
	{"text_replay", P_OFFINT (pchat_text_replay), TYPE_BOOL},
	{"text_search_case_match", P_OFFINT (pchat_text_search_case_match), TYPE_BOOL},
	{"text_search_highlight_all", P_OFFINT (pchat_text_search_highlight_all), TYPE_BOOL},
	{"text_search_follow", P_OFFINT (pchat_text_search_follow), TYPE_BOOL},
	{"text_search_regexp", P_OFFINT (pchat_text_search_regexp), TYPE_BOOL},
	{"text_show_marker", P_OFFINT (pchat_text_show_marker), TYPE_BOOL},
	{"text_show_sep", P_OFFINT (pchat_text_show_sep), TYPE_BOOL},
	{"text_spell_langs", P_OFFSET (pchat_text_spell_langs), TYPE_STR},
	{"text_stripcolor_msg", P_OFFINT (pchat_text_stripcolor_msg), TYPE_BOOL},
	{"text_stripcolor_replay", P_OFFINT (pchat_text_stripcolor_replay), TYPE_BOOL},
	{"text_stripcolor_topic", P_OFFINT (pchat_text_stripcolor_topic), TYPE_BOOL},
	{"text_thin_sep", P_OFFINT (pchat_text_thin_sep), TYPE_BOOL},
	{"text_transparent", P_OFFINT (pchat_text_transparent), TYPE_BOOL},
	{"text_wordwrap", P_OFFINT (pchat_text_wordwrap), TYPE_BOOL},

	{"url_grabber", P_OFFINT (pchat_url_grabber), TYPE_BOOL},
	{"url_grabber_limit", P_OFFINT (pchat_url_grabber_limit), TYPE_INT},
	{"url_logging", P_OFFINT (pchat_url_logging), TYPE_BOOL},
	{0, 0, 0},
};

static char *
convert_with_fallback (const char *str, const char *fallback)
{
	char *utf;

	utf = g_locale_to_utf8 (str, -1, 0, 0, 0);
	if (!utf)
	{
		/* this can happen if CHARSET envvar is set wrong */
		/* maybe it's already utf8 (breakage!) */
		if (!g_utf8_validate (str, -1, NULL))
			utf = g_strdup (fallback);
		else
			utf = g_strdup (str);
	}

	return utf;
}

static int
find_language_number (const char * const lang)
{
	int i;

	for (i = 0; i < LANGUAGES_LENGTH; i++)
		if (!strcmp (lang, languages[i]))
			return i;

	return -1;
}

/* Return the number of the system language if found, or english otherwise.
 */
static int
get_default_language (void)
{
	const char *locale;
	char *lang;
	char *p;
	int lang_no;

	/* LC_ALL overrides LANG, so we must check it first */
	locale = g_getenv ("LC_ALL");

	if (!locale)
		locale = g_getenv ("LANG") ? g_getenv ("LANG") : "en";

	/* we might end up with something like "en_US.UTF-8".  We will try to
	 * search for "en_US"; if it fails we search for "en".
	 */
	lang = g_strdup (locale);

	if ((p = strchr (lang, '.')))
		*p='\0';

	lang_no = find_language_number (lang);

	if (lang_no >= 0)
	{
		g_free (lang);
		return lang_no;
	}

	if ((p = strchr (lang, '_')))
		*p='\0';

	lang_no = find_language_number (lang);

	g_free (lang);

	return lang_no >= 0 ? lang_no : find_language_number ("en");
}

static char *
get_default_spell_languages (void)
{
	const gchar* const *langs = g_get_language_names ();
	char *last = NULL;
	char *p;
	char lang_list[64];
	char *ret = lang_list;
	int i;

	if (langs != NULL)
	{
		memset (lang_list, 0, sizeof(lang_list));

		for (i = 0; langs[i]; i++)
		{
			if (g_ascii_strncasecmp (langs[i], "C", 1) != 0 && strlen (langs[i]) >= 2)
			{
				/* Avoid duplicates */
				if (!last || !g_str_has_prefix (langs[i], last))
				{
					if (last != NULL)
					{
						g_free(last);
						g_strlcat (lang_list, ",", sizeof(lang_list));
					}

					/* ignore .utf8 */
					if ((p = strchr (langs[i], '.')))
						*p='\0';

					last = g_strndup (langs[i], 2);

					g_strlcat (lang_list, langs[i], sizeof(lang_list));
				}
			}
		}
		g_free (last);

		if (lang_list[0])
			return g_strdup (ret);
	}

	return g_strdup ("en");
}

void
load_default_config(void)
{
	const char *username, *realname, *font;
	char *sp;
#ifdef _WIN32
	char out[256];
#endif

	username = g_strdup(g_get_user_name ());
	if (!username)
		username = g_strdup ("root");

	/* We hid Real name from the Network List, so don't use the user's name unnoticeably */
	/* realname = g_get_real_name ();
	if ((realname && realname[0] == 0) || !realname)
		realname = username; */
	realname = g_strdup ("realname");

	username = convert_with_fallback (username, "username");
	realname = convert_with_fallback (realname, "realname");

	memset (&prefs, 0, sizeof (struct xchatprefs));

	/* put in default values, anything left out is automatically zero */

	/* BOOLEANS */
	prefs.pchat_away_show_once = 1;
	prefs.pchat_away_track = 1;
	prefs.pchat_dcc_auto_resume = 1;
#ifndef _WIN32
	prefs.pchat_dcc_fast_send = 1;
#endif
	prefs.pchat_gui_autoopen_chat = 1;
	prefs.pchat_gui_autoopen_dialog = 1;
	prefs.pchat_gui_autoopen_recv = 1;
	prefs.pchat_gui_autoopen_send = 1;
#ifdef HAVE_GTK_MAC
	prefs.pchat_gui_hide_menu = 1;
#endif
	prefs.pchat_gui_input_attr = 1;
	prefs.pchat_gui_input_icon = 1;
	prefs.pchat_gui_input_nick = 1;
	prefs.pchat_gui_input_spell = 1;
	prefs.pchat_gui_input_style = 1;
	prefs.pchat_gui_join_dialog = 1;
	prefs.pchat_gui_quit_dialog = 1;
	/* prefs.pchat_gui_slist_skip = 1; */
	prefs.pchat_gui_tab_chans = 1;
	prefs.pchat_gui_tab_dialogs = 1;
	prefs.pchat_gui_tab_dots = 1;
	prefs.pchat_gui_tab_icons = 1;
	prefs.pchat_gui_tab_server = 1;
	prefs.pchat_gui_tab_sort = 1;
	prefs.pchat_gui_topicbar = 1;
	prefs.pchat_gui_transparency = 255;
	prefs.pchat_gui_tray = 1;
	prefs.pchat_gui_tray_blink = 1;
	prefs.pchat_gui_ulist_count = 1;
	prefs.pchat_gui_ulist_icons = 1;
	prefs.pchat_gui_ulist_resizable = 1;
	prefs.pchat_gui_ulist_style = 1;
	prefs.pchat_gui_win_save = 1;
	prefs.pchat_identd = 1;
	prefs.pchat_input_flash_hilight = 1;
	prefs.pchat_input_flash_priv = 1;
	prefs.pchat_input_tray_hilight = 1;
	prefs.pchat_input_tray_priv = 1;
	prefs.pchat_irc_who_join = 1; /* Can kick with inordinate amount of channels, required for some of our features though, TODO: add cap like away check? */
	prefs.pchat_irc_whois_front = 1;
	prefs.pchat_net_auto_reconnect = 1;
	prefs.pchat_net_throttle = 1;
	prefs.pchat_stamp_log = 1;
	prefs.pchat_stamp_text = 1;
	prefs.pchat_text_autocopy_text = 1;
	prefs.pchat_text_indent = 1;
	prefs.pchat_text_replay = 1;
	prefs.pchat_text_search_follow = 1;
	prefs.pchat_text_show_marker = 1;
	prefs.pchat_text_show_sep = 1;
	prefs.pchat_text_stripcolor_replay = 1;
	prefs.pchat_text_stripcolor_topic = 1;
	prefs.pchat_text_thin_sep = 1;
	prefs.pchat_text_wordwrap = 1;
	prefs.pchat_url_grabber = 1;
	prefs.pchat_irc_cap_server_time = 0;

	/* NUMBERS */
	prefs.pchat_away_size_max = 300;
	prefs.pchat_away_timeout = 60;
	prefs.pchat_completion_amount = 5;
	prefs.pchat_dcc_auto_recv = 1;			/* browse mode */
	prefs.pchat_dcc_blocksize = 1024;
	prefs.pchat_dcc_permissions = 0600;
	prefs.pchat_dcc_stall_timeout = 60;
	prefs.pchat_dcc_timeout = 180;
	prefs.pchat_flood_ctcp_num = 5;
	prefs.pchat_flood_ctcp_time = 30;
	prefs.pchat_flood_msg_num = 5;
	/*FIXME*/ prefs.pchat_flood_msg_time = 30;
	prefs.pchat_gui_chanlist_maxusers = 9999;
	prefs.pchat_gui_chanlist_minusers = 5;
	prefs.pchat_gui_dialog_height = 256;
	prefs.pchat_gui_dialog_width = 500;
	prefs.pchat_gui_lagometer = 1;
	prefs.pchat_gui_lang = get_default_language();
	prefs.pchat_gui_pane_left_size = 128;		/* with treeview icons we need a bit bigger space */
	prefs.pchat_gui_pane_right_size = 100;
	prefs.pchat_gui_pane_right_size_min = 80;
	prefs.pchat_gui_tab_layout = 2;			/* 0=Tabs 1=Reserved 2=Tree */
	prefs.pchat_gui_tab_newtofront = 2;
	prefs.pchat_gui_tab_pos = 1;
	prefs.pchat_gui_tab_trunc = 20;
	prefs.pchat_gui_throttlemeter = 1;
	prefs.pchat_gui_ulist_pos = 3;
	prefs.pchat_gui_win_height = 400;
	prefs.pchat_gui_win_width = 640;
	prefs.pchat_input_balloon_time = 20;
	prefs.pchat_irc_ban_type = 1;
	prefs.pchat_irc_join_delay = 5;
	prefs.pchat_net_reconnect_delay = 10;
	prefs.pchat_notify_timeout = 15;
	prefs.pchat_text_max_indent = 256;
	prefs.pchat_text_max_lines = 500;
	prefs.pchat_url_grabber_limit = 100; 		/* 0 means unlimited */

	/* STRINGS */
	strcpy (prefs.pchat_away_reason, _("I'm busy"));
	strcpy (prefs.pchat_completion_suffix, ",");
#ifdef _WIN32
	if (portable_mode () || !get_reg_str ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal", out, sizeof (out)))
	{
		g_snprintf (prefs.pchat_dcc_dir, sizeof (prefs.pchat_dcc_dir), "%s\\downloads", get_xdir ());
	}
	else
	{
		snprintf (prefs.pchat_dcc_dir, sizeof (prefs.pchat_dcc_dir), "%s\\Downloads", out);
	}
#else
	if (g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD))
	{
		strcpy (prefs.pchat_dcc_dir, g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD));
	}
	else
	{
		strcpy (prefs.pchat_dcc_dir, g_build_filename (g_get_home_dir (), "Downloads", NULL));
	}
#endif
	strcpy (prefs.pchat_gui_ulist_doubleclick, "QUERY %s");
	strcpy (prefs.pchat_input_command_char, "/");
	strcpy (prefs.pchat_irc_logmask, g_build_filename ("%n", "%c.log", NULL));
	strcpy (prefs.pchat_irc_nick1, username);
	strcpy (prefs.pchat_irc_nick2, username);
	strcat (prefs.pchat_irc_nick2, "_");
	strcpy (prefs.pchat_irc_nick3, username);
	strcat (prefs.pchat_irc_nick3, "__");
	strcpy (prefs.pchat_irc_no_hilight, "NickServ,ChanServ,InfoServ,N,Q");
	strcpy (prefs.pchat_irc_part_reason, _("Leaving"));
	strcpy (prefs.pchat_irc_quit_reason, prefs.pchat_irc_part_reason);
	strcpy (prefs.pchat_irc_real_name, realname);
	strcpy (prefs.pchat_irc_user_name, username);
	strcpy (prefs.pchat_stamp_log_format, "%b %d %H:%M:%S ");
	strcpy (prefs.pchat_stamp_text_format, "[%H:%M:%S] ");

	font = fe_get_default_font ();
	if (font)
	{
		strcpy (prefs.pchat_text_font, font);
		strcpy (prefs.pchat_text_font_main, font);
	}
	else
	{
		strcpy (prefs.pchat_text_font, DEF_FONT);
		strcpy (prefs.pchat_text_font_main, DEF_FONT);
	}

	strcpy (prefs.pchat_text_font_alternative, DEF_FONT_ALTER);
	strcpy (prefs.pchat_text_spell_langs, get_default_spell_languages ());


	/* private variables */
	prefs.local_ip = 0xffffffff;

	sp = strchr (prefs.pchat_irc_user_name, ' ');
	if (sp)
		sp[0] = 0;	/* spaces in username would break the login */

	g_free (username);
	g_free (realname);
}

int
make_config_dirs (void)
{
	char *buf;

	if (g_mkdir_with_parents (get_xdir (), 0700) != 0)
		return -1;

	buf = g_build_filename (get_xdir (), "addons", NULL);
	if (g_mkdir (buf, 0700) != 0)
	{
		g_free (buf);
		return -1;
	}
	g_free (buf);

	buf = g_build_filename (get_xdir (), XCHAT_SOUND_DIR, NULL);
	if (g_mkdir (buf, 0700) != 0)
	{
		g_free (buf);
		return -1;
	}
	g_free (buf);

	return 0;
}

int
make_dcc_dirs (void)
{
	if (g_mkdir (prefs.pchat_dcc_dir, 0700) != 0)
		return -1;

	if (g_mkdir (prefs.pchat_dcc_completed_dir, 0700) != 0)
		return -1;

	return 0;
}

int
load_config (void)
{
	char *cfg, *sp;
	int res, val, i;

	g_assert(check_config_dir () == 0);

	if (!g_file_get_contents (default_file (), &cfg, NULL, NULL))
		return -1;

	/* If the config is incomplete we have the default values loaded */
	load_default_config();

	i = 0;
	do
	{
		switch (vars[i].type)
		{
		case TYPE_STR:
			cfg_get_str (cfg, vars[i].name, (char *) &prefs + vars[i].offset,
				     vars[i].len);
			break;
		case TYPE_BOOL:
		case TYPE_INT:
			val = cfg_get_int_with_result (cfg, vars[i].name, &res);
			if (res)
				*((int *) &prefs + vars[i].offset) = val;
			break;
		}
		i++;
	}
	while (vars[i].name);

	g_free (cfg);

	if (prefs.pchat_gui_win_height < 138)
		prefs.pchat_gui_win_height = 138;
	if (prefs.pchat_gui_win_width < 106)
		prefs.pchat_gui_win_width = 106;

	sp = strchr (prefs.pchat_irc_user_name, ' ');
	if (sp)
		sp[0] = 0;	/* spaces in username would break the login */

	return 0;
}

int
save_config (void)
{
	int fh, i;
	char *config, *new_config;

	if (check_config_dir () != 0)
		make_config_dirs ();

	config = default_file ();
	new_config = g_strconcat (config, ".new", NULL);

	fh = g_open (new_config, OFLAGS | O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh == -1)
	{
		g_free (new_config);
		return 0;
	}

	if (!cfg_put_str (fh, "version", PACKAGE_VERSION))
	{
                close (fh);
		g_free (new_config);
		return 0;
	}

	i = 0;
	do
	{
		switch (vars[i].type)
		{
		case TYPE_STR:
			if (!cfg_put_str (fh, vars[i].name, (char *) &prefs + vars[i].offset))
			{
                                close (fh);
				g_free (new_config);
				return 0;
			}
			break;
		case TYPE_INT:
		case TYPE_BOOL:
			if (!cfg_put_int (fh, *((int *) &prefs + vars[i].offset), vars[i].name))
			{
                                close (fh);
				g_free (new_config);
				return 0;
			}
		}
		i++;
	}
	while (vars[i].name);

	if (close (fh) == -1)
	{
		g_free (new_config);
		return 0;
	}

#ifdef _WIN32
	g_unlink (config);	/* win32 can't rename to an existing file */
#endif
	if (g_rename (new_config, config) == -1)
	{
		g_free (new_config);
		return 0;
	}
	g_free (new_config);

	return 1;
}

static void
set_showval (session *sess, const struct prefs *var, char *tbuf)
{
	size_t len;
	size_t dots;
	size_t j;

	len = strlen (var->name);
	memcpy (tbuf, var->name, len);
	if (len > 29)
		dots = 0;
	else
		dots = 29 - len;

	tbuf[len++] = '\003';
	tbuf[len++] = '2';

	for (j = 0; j < dots; j++)
	{
		tbuf[j + len] = '.';
	}

	len += j;

	switch (var->type)
	{
		case TYPE_STR:
			sprintf (tbuf + len, "\0033:\017 %s\n", (char *) &prefs + var->offset);
			break;
		case TYPE_INT:
			sprintf (tbuf + len, "\0033:\017 %d\n", *((int *) &prefs + var->offset));
			break;
		case TYPE_BOOL:
			if (*((int *) &prefs + var->offset))
			{
				sprintf (tbuf + len, "\0033:\017 %s\n", "ON");
			}
			else
			{
				sprintf (tbuf + len, "\0033:\017 %s\n", "OFF");
			}
			break;
	}

	PrintText (sess, tbuf);
}

static void
set_list (session * sess, char *tbuf)
{
	int i;

	i = 0;
	do
	{
		set_showval (sess, &vars[i], tbuf);
		i++;
	}
	while (vars[i].name);
}

int
cfg_get_bool (char *var)
{
	int i = 0;

	do
	{
		if (!g_ascii_strcasecmp (var, vars[i].name))
		{
			return *((int *) &prefs + vars[i].offset);
		}
		i++;
	}
	while (vars[i].name);

	return -1;
}

int
cmd_set (struct session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int wild = FALSE;
	int or = FALSE;
	int off = FALSE;
	int quiet = FALSE;
	int erase = FALSE;
	int i = 0, finds = 0, found;
	int idx = 2;
	int prev_numeric;
	char *var, *val, *prev_string;

	if (g_ascii_strcasecmp (word[2], "-e") == 0)
	{
		idx++;
		erase = TRUE;
	}

	/* turn a bit OFF */
	if (g_ascii_strcasecmp (word[idx], "-off") == 0)
	{
		idx++;
		off = TRUE;
	}

	/* turn a bit ON */
	if (g_ascii_strcasecmp (word[idx], "-or") == 0 || g_ascii_strcasecmp (word[idx], "-on") == 0)
	{
		idx++;
		or = TRUE;
	}

	if (g_ascii_strcasecmp (word[idx], "-quiet") == 0)
	{
		idx++;
		quiet = TRUE;
	}

	var = word[idx];
	val = word_eol[idx+1];

	if (!*var)
	{
		set_list (sess, tbuf);
		return TRUE;
	}

	if ((strchr (var, '*') || strchr (var, '?')) && !*val)
	{
		wild = TRUE;
	}

	if (*val == '=')
	{
		val++;
	}

	do
	{
		if (wild)
		{
			found = !match (var, vars[i].name);
		}
		else
		{
			found = g_ascii_strcasecmp (var, vars[i].name);
		}

		if (found == 0)
		{
			finds++;
			switch (vars[i].type)
			{
			case TYPE_STR:
				if (erase || *val)
				{
					/* save the previous value until we print it out */
					prev_string = g_malloc (vars[i].len + 1);
					strncpy (prev_string, (char *) &prefs + vars[i].offset, vars[i].len);

					/* update the variable */
					strncpy ((char *) &prefs + vars[i].offset, val, vars[i].len);
					((char *) &prefs)[vars[i].offset + vars[i].len - 1] = 0;

					if (!quiet)
					{
						PrintTextf (sess, "%s set to: %s (was: %s)\n", var, (char *) &prefs + vars[i].offset, prev_string);
					}

					g_free (prev_string);
				}
				else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			case TYPE_INT:
			case TYPE_BOOL:
				if (*val)
				{
					prev_numeric = *((int *) &prefs + vars[i].offset);
					if (vars[i].type == TYPE_BOOL)
					{
						if (atoi (val))
						{
							*((int *) &prefs + vars[i].offset) = 1;
						}
						else
						{
							*((int *) &prefs + vars[i].offset) = 0;
						}
						if (!g_ascii_strcasecmp (val, "YES") || !g_ascii_strcasecmp (val, "ON"))
						{
							*((int *) &prefs + vars[i].offset) = 1;
						}
						if (!g_ascii_strcasecmp (val, "NO") || !g_ascii_strcasecmp (val, "OFF"))
						{
							*((int *) &prefs + vars[i].offset) = 0;
						}
					}
					else
					{
						if (or)
						{
							*((int *) &prefs + vars[i].offset) |= atoi (val);
						}
						else if (off)
						{
							*((int *) &prefs + vars[i].offset) &= ~(atoi (val));
						}
						else
						{
							*((int *) &prefs + vars[i].offset) = atoi (val);
						}
					}
					if (!quiet)
					{
						PrintTextf (sess, "%s set to: %d (was: %d)\n", var, *((int *) &prefs + vars[i].offset), prev_numeric);
					}
				}
				else
				{
					set_showval (sess, &vars[i], tbuf);
				}
				break;
			}
		}
		i++;
	}
	while (vars[i].name);

	if (!finds && !quiet)
	{
		PrintText (sess, "No such variable.\n");
	}
	else if (!save_config ())
	{
		PrintText (sess, "Error saving changes to disk.\n");
	}

	return TRUE;
}

int
xchat_open_file (char *file, int flags, int mode, int xof_flags)
{
	char *buf;
	int fd;

	if (xof_flags & XOF_FULLPATH)
	{
		if (xof_flags & XOF_DOMODE)
			return g_open (file, flags | OFLAGS, mode);
		else
			return g_open (file, flags | OFLAGS, 0);
	}

	buf = g_build_filename (get_xdir (), file, NULL);

	if (xof_flags & XOF_DOMODE)
	{
		fd = g_open (buf, flags | OFLAGS, mode);
	}
	else
	{
		fd = g_open (buf, flags | OFLAGS, 0);
	}

	g_free (buf);

	return fd;
}

FILE *
xchat_fopen_file (const char *file, const char *mode, int xof_flags)
{
	char *buf;
	FILE *fh;

	if (xof_flags & XOF_FULLPATH)
		return g_fopen (file, mode);

	buf = g_build_filename (get_xdir (), file, NULL);
	fh = g_fopen (buf, mode);
	g_free (buf);

	return fh;
}
