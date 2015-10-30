/* HexChat
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

#ifndef HEXCHAT_GTKCOMPAT_H
#define HEXCHAT_GTKCOMPAT_H

/* chanview-tabs.c */
#if GTK_CHECK_VERSION (3, 0, 0)
#define compat_gdk_window_get_geometry(win, x, y, wid, high, dep) \
			gdk_window_get_geometry(win, x, y, wid, high)
#else
#define compat_gdk_window_get_geometry(win, x, y, wid, high, dep) \
			gdk_window_get_geometry(win, x, y, wid, high, dep)
#endif

/* xtext.c */
#if !GTK_CHECK_VERSION(3, 0, 0)
#define gtk_widget_get_allocated_height(widget)((GTK_WIDGET(widget))->GSEAL(allocation).height)
#define gtk_widget_get_allocated_width(widget)((GTK_WIDGET(widget))->GSEAL(allocation).width)
#endif

#endif /* HEXCHAT_GTKCOMPAT_H */
