/* PChat
 * Copyright (C) 2025 PChat Contributors
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

#include <libpeas.h>
#include <glib.h>

#include "pchat.h"
#include "plugin-peas.h"
#include "cfgfiles.h"
#include "text.h"

static PeasEngine *peas_engine = NULL;
static char *last_error_message = NULL;

static void
on_plugin_loaded (PeasEngine *engine,
                  PeasPluginInfo *info,
                  gpointer user_data)
{
	const gchar *name = peas_plugin_info_get_name (info);
	g_print ("Libpeas plugin loaded: %s\n", name);
}

static void
on_plugin_unloaded (PeasEngine *engine,
                    PeasPluginInfo *info,
                    gpointer user_data)
{
	const gchar *name = peas_plugin_info_get_name (info);
	g_print ("Libpeas plugin unloaded: %s\n", name);
}

void
plugin_peas_init (void)
{
	if (peas_engine != NULL)
		return;

	/* Get the default engine */
	peas_engine = peas_engine_get_default ();

	/* Enable Python3 loader */
	peas_engine_enable_loader (peas_engine, "python3");

	/* Connect signals for plugin loading/unloading */
	g_signal_connect (peas_engine, "load-plugin",
	                  G_CALLBACK (on_plugin_loaded), NULL);
	g_signal_connect (peas_engine, "unload-plugin",
	                  G_CALLBACK (on_plugin_unloaded), NULL);

	g_print ("Libpeas initialized\\n");
}

void
plugin_peas_cleanup (void)
{
	if (peas_engine == NULL)
		return;

	/* In libpeas 2, we need to manually track loaded plugins or iterate differently */
	/* For now, just clear the engine - plugins will be unloaded automatically */
	g_clear_object (&peas_engine);
}

void
plugin_peas_auto_load (void)
{
	gchar *user_plugin_dir;
	gchar *system_plugin_dir;

	if (peas_engine == NULL)
		return;

	/* User plugin directory: ~/.config/pchat/plugins */
	user_plugin_dir = g_build_filename (get_xdir (), "plugins", NULL);

	/* System plugin directory */
#ifdef WIN32
	/* Windows: use PCHAT_LIBDIR/plugins */
	system_plugin_dir = g_build_filename (PCHATLIBDIR, "plugins", NULL);
#else
	/* Unix: use standard plugin directory */
	system_plugin_dir = g_build_filename (PCHATLIBDIR, "plugins", NULL);
#endif

	/* Add search paths */
	peas_engine_add_search_path (peas_engine, user_plugin_dir, NULL);
	peas_engine_add_search_path (peas_engine, system_plugin_dir, NULL);

	g_print ("Libpeas plugin paths:\n");
	g_print ("  User: %s\n", user_plugin_dir);
	g_print ("  System: %s\n", system_plugin_dir);

	g_free (user_plugin_dir);
	g_free (system_plugin_dir);

	/* In libpeas 2, plugins are discovered via rescan and loaded individually */
	/* Rescan the plugin directories */
	peas_engine_rescan_plugins (peas_engine);

	/* Note: In libpeas 2, automatic plugin loading is typically handled differently.
	 * For now, we rely on the engine discovering plugins. To auto-load specific plugins,
	 * you would need to use peas_engine_load_plugin() with known plugin module names.
	 * Example: peas_engine_load_plugin(peas_engine, 
	 *          peas_engine_get_plugin_info(peas_engine, "example"));
	 */
	g_print ("Libpeas plugin discovery complete. Use /PLUGIN command to load plugins.\\n");
}

PeasEngine *
plugin_peas_get_engine (void)
{
	return peas_engine;
}

gboolean
plugin_peas_load (const char *module_name)
{
	PeasPluginInfo *info;
	GError *error = NULL;
	
	g_free (last_error_message);
	last_error_message = NULL;

	if (peas_engine == NULL)
	{
		last_error_message = g_strdup ("Libpeas engine not initialized!");
		return FALSE;
	}

	info = peas_engine_get_plugin_info (peas_engine, module_name);
	if (!info)
	{
		last_error_message = g_strdup_printf ("Plugin '%s' not found in search paths", module_name);
		return FALSE;
	}

	if (peas_plugin_info_is_loaded (info))
	{
		last_error_message = g_strdup_printf ("Plugin '%s' is already loaded", module_name);
		return TRUE;
	}

	if (!peas_plugin_info_is_available (info, &error))
	{
		if (error)
		{
			last_error_message = g_strdup_printf ("Plugin not available: %s", error->message);
			g_clear_error (&error);
		}
		else
		{
			const char *loader = peas_plugin_info_get_external_data (info, "Loader");
			if (loader && g_strcmp0 (loader, "python3") == 0)
			{
				last_error_message = g_strdup ("Python3 loader required. Install: pacman -S mingw-w64-*-python-gobject");
			}
			else
			{
				last_error_message = g_strdup_printf ("Plugin not available (loader: %s)", loader ? loader : "unknown");
			}
		}
		return FALSE;
	}

	if (peas_engine_load_plugin (peas_engine, info))
	{
		return TRUE;
	}

	last_error_message = g_strdup_printf ("Failed to load plugin '%s' (unknown error)", module_name);
	return FALSE;
}

const char *
plugin_peas_get_last_error (void)
{
	return last_error_message;
}

gboolean
plugin_peas_unload (const char *module_name)
{
	PeasPluginInfo *info;

	if (peas_engine == NULL)
		return FALSE;

	info = peas_engine_get_plugin_info (peas_engine, module_name);
	if (!info)
	{
		g_warning ("Libpeas plugin not found: %s", module_name);
		return FALSE;
	}

	if (!peas_plugin_info_is_loaded (info))
	{
		g_print ("Libpeas plugin not loaded: %s\\n", module_name);
		return TRUE;
	}

	if (peas_engine_unload_plugin (peas_engine, info))
	{
		g_print ("Unloaded libpeas plugin: %s\\n", peas_plugin_info_get_name (info));
		return TRUE;
	}

	g_warning ("Failed to unload libpeas plugin: %s", module_name);
	return FALSE;
}
