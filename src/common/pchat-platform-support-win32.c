/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 *
 * PChat
 * Copyright (C) 2025 Zach Bacon
 *
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
#include "pchat-platform-support.h"

#include <glib.h>
//#include <gio/gwin32inputstream.h>

#define SAVE_DATADIR DATADIR
#undef DATADIR
#include <io.h>
#include <conio.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#define DATADIR SAVE_DATADIR
#undef SAVE_DATADIR



char *
pchat_platform_support_get_lib_dir (void)
{
	char *module_dir;
	char *lib_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	lib_dir = g_build_filename (module_dir, "lib", "pchat", NULL);
	g_free (module_dir);

	return lib_dir;
}

char *
pchat_platform_support_get_locale_dir (void)
{
	char *module_dir;
	char *locale_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	locale_dir = g_build_filename (module_dir, "share", "locale", NULL);
	g_free (module_dir);

	return locale_dir;
}

char *
pchat_platform_support_get_data_dir (void)
{
	char *module_dir;
	char *data_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	data_dir = g_build_filename (module_dir, "share", "pchat", NULL);
	g_free (module_dir);

	return data_dir;
}

char *
pchat_platform_support_get_user_home_dir (const char *name)
{
	// TODO
	return NULL;
}

