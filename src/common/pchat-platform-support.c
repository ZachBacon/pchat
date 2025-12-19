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

/*
 * This file is inspired by gitg's platform support code and will allow a more fhs install on a variety of platforms
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include "pchat-platform-support.h"

char *
pchat_platform_support_get_lib_dir (void)
{
	return g_strdup (XCHATLIBDIR);
}

char *
pchat_platform_support_get_locale_dir (void)
{
	return g_strdup (ISO_CODES_LOCALEDIR);
}

char *
pchat_platform_support_get_data_dir (void)
{
	return g_strdup (XCHATSHAREDIR);
}

char *
pchat_platform_support_get_user_home_dir (const char *name)
{
	return 0;
}
