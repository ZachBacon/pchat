/* PChat - IRC Client - CSS helper functions
 * Copyright (C) 2025 Zach Bacon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PCHAT_CSS_HELPERS_H
#define PCHAT_CSS_HELPERS_H

#include <gtk/gtk.h>

/* Helper function to convert Pango font description to CSS */
static inline gchar *
pango_font_description_to_css (PangoFontDescription *desc)
{
	const char *family = pango_font_description_get_family (desc);
	gint size = pango_font_description_get_size (desc);
	PangoWeight weight = pango_font_description_get_weight (desc);
	PangoStyle style = pango_font_description_get_style (desc);
	GString *css = g_string_new ("");
	
	/* Quote font family to handle names with spaces or special characters */
	if (family)
		g_string_append_printf (css, "font-family: '%s';", family);
	
	if (size > 0)
	{
		if (pango_font_description_get_size_is_absolute (desc))
			g_string_append_printf (css, " font-size: %dpx;", size);
		else
			g_string_append_printf (css, " font-size: %dpt;", size / PANGO_SCALE);
	}
	
	if (weight != PANGO_WEIGHT_NORMAL)
		g_string_append_printf (css, " font-weight: %d;", weight);
	
	if (style == PANGO_STYLE_ITALIC)
		g_string_append (css, " font-style: italic;");
	else if (style == PANGO_STYLE_OBLIQUE)
		g_string_append (css, " font-style: oblique;");
	
	return g_string_free (css, FALSE);
}

#endif /* PCHAT_CSS_HELPERS_H */