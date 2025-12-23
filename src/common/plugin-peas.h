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

#ifndef PCHAT_PLUGIN_PEAS_H
#define PCHAT_PLUGIN_PEAS_H

#ifdef USE_LIBPEAS

#include <libpeas.h>

/* Initialize libpeas plugin engine */
void plugin_peas_init (void);

/* Cleanup libpeas plugin engine */
void plugin_peas_cleanup (void);

/* Auto-load libpeas plugins from directories */
void plugin_peas_auto_load (void);

/* Get the global peas engine instance */
PeasEngine *plugin_peas_get_engine (void);

/* Load a specific libpeas plugin by module name */
gboolean plugin_peas_load (const char *module_name);

/* Unload a specific libpeas plugin by module name */
gboolean plugin_peas_unload (const char *module_name);

/* Get the last error message from plugin operations */
const char *plugin_peas_get_last_error (void);

#endif /* USE_LIBPEAS */

#endif /* PCHAT_PLUGIN_PEAS_H */
