/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * PChat
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "fe-gtk.h"
#include "palette.h"

#include "../common/pchat.h"
#include "../common/util.h"
#include "../common/cfgfiles.h"
#include "../common/typedef.h"


GdkRGBA colors[] = {
	/* colors for xtext - converted from GdkColor (16-bit) to GdkRGBA (double) */
	{0xd3d3/65535.0, 0xd7d7/65535.0, 0xcfcf/65535.0, 1.0}, /* 0 white */
	{0x2e2e/65535.0, 0x3434/65535.0, 0x3636/65535.0, 1.0}, /* 1 black */
	{0x3434/65535.0, 0x6565/65535.0, 0xa4a4/65535.0, 1.0}, /* 2 blue */
	{0x4e4e/65535.0, 0x9a9a/65535.0, 0x0606/65535.0, 1.0}, /* 3 green */
	{0xcccc/65535.0, 0x0000/65535.0, 0x0000/65535.0, 1.0}, /* 4 red */
	{0x8f8f/65535.0, 0x3939/65535.0, 0x0202/65535.0, 1.0}, /* 5 light red */
	{0x5c5c/65535.0, 0x3535/65535.0, 0x6666/65535.0, 1.0}, /* 6 purple */
	{0xcece/65535.0, 0x5c5c/65535.0, 0x0000/65535.0, 1.0}, /* 7 orange */
	{0xc4c4/65535.0, 0xa0a0/65535.0, 0x0000/65535.0, 1.0}, /* 8 yellow */
	{0x7373/65535.0, 0xd2d2/65535.0, 0x1616/65535.0, 1.0}, /* 9 green */
	{0x1111/65535.0, 0xa8a8/65535.0, 0x7979/65535.0, 1.0}, /* 10 aqua */
	{0x5858/65535.0, 0xa1a1/65535.0, 0x9d9d/65535.0, 1.0}, /* 11 light aqua */
	{0x5757/65535.0, 0x7979/65535.0, 0x9e9e/65535.0, 1.0}, /* 12 blue */
	{0xa0d0/65535.0, 0x42d4/65535.0, 0x6562/65535.0, 1.0}, /* 13 light purple */
	{0x5555/65535.0, 0x5757/65535.0, 0x5353/65535.0, 1.0}, /* 14 grey */
	{0x8888/65535.0, 0x8a8a/65535.0, 0x8585/65535.0, 1.0}, /* 15 light grey */

	{0xd3d3/65535.0, 0xd7d7/65535.0, 0xcfcf/65535.0, 1.0}, /* 16 white */
	{0x2e2e/65535.0, 0x3434/65535.0, 0x3636/65535.0, 1.0}, /* 17 black */
	{0x3434/65535.0, 0x6565/65535.0, 0xa4a4/65535.0, 1.0}, /* 18 blue */
	{0x4e4e/65535.0, 0x9a9a/65535.0, 0x0606/65535.0, 1.0}, /* 19 green */
	{0xcccc/65535.0, 0x0000/65535.0, 0x0000/65535.0, 1.0}, /* 20 red */
	{0x8f8f/65535.0, 0x3939/65535.0, 0x0202/65535.0, 1.0}, /* 21 light red */
	{0x5c5c/65535.0, 0x3535/65535.0, 0x6666/65535.0, 1.0}, /* 22 purple */
	{0xcece/65535.0, 0x5c5c/65535.0, 0x0000/65535.0, 1.0}, /* 23 orange */
	{0xc4c4/65535.0, 0xa0a0/65535.0, 0x0000/65535.0, 1.0}, /* 24 yellow */
	{0x7373/65535.0, 0xd2d2/65535.0, 0x1616/65535.0, 1.0}, /* 25 green */
	{0x1111/65535.0, 0xa8a8/65535.0, 0x7979/65535.0, 1.0}, /* 26 aqua */
	{0x5858/65535.0, 0xa1a1/65535.0, 0x9d9d/65535.0, 1.0}, /* 27 light aqua */
	{0x5757/65535.0, 0x7979/65535.0, 0x9e9e/65535.0, 1.0}, /* 28 blue */
	{0xa0d0/65535.0, 0x42d4/65535.0, 0x6562/65535.0, 1.0}, /* 29 light purple */
	{0x5555/65535.0, 0x5757/65535.0, 0x5353/65535.0, 1.0}, /* 30 grey */
	{0x8888/65535.0, 0x8a8a/65535.0, 0x8585/65535.0, 1.0}, /* 31 light grey */

	{0xd3d3/65535.0, 0xd7d7/65535.0, 0xcfcf/65535.0, 1.0}, /* 32 marktext Fore (white) */
	{0x2020/65535.0, 0x4a4a/65535.0, 0x8787/65535.0, 1.0}, /* 33 marktext Back (blue) */
	{0x2512/65535.0, 0x29e8/65535.0, 0x2b85/65535.0, 1.0}, /* 34 foreground (black) */
	{0xfae0/65535.0, 0xfae0/65535.0, 0xf8c4/65535.0, 1.0}, /* 35 background (white) */
	{0x8f8f/65535.0, 0x3939/65535.0, 0x0202/65535.0, 1.0}, /* 36 marker line (red) */

	/* colors for GUI */
	{0x3434/65535.0, 0x6565/65535.0, 0xa4a4/65535.0, 1.0}, /* 37 tab New Data (dark red) */
	{0x4e4e/65535.0, 0x9a9a/65535.0, 0x0606/65535.0, 1.0}, /* 38 tab Nick Mentioned (blue) */
	{0xcece/65535.0, 0x5c5c/65535.0, 0x0000/65535.0, 1.0}, /* 39 tab New Message (red) */
	{0x8888/65535.0, 0x8a8a/65535.0, 0x8585/65535.0, 1.0}, /* 40 away user (grey) */
	{0xa4a4/65535.0, 0x0000/65535.0, 0x0000/65535.0, 1.0}, /* 41 spell checker color (red) */
};


void
palette_alloc (GtkWidget * widget)
{
#if 0 /* FIXME: Use GdkVisual */
	int i;
	static int done_alloc = FALSE;
	GdkColormap *cmap;

	if (!done_alloc)		  /* don't do it again */
	{
		done_alloc = TRUE;
		cmap = gtk_widget_get_colormap (widget);
		for (i = MAX_COL; i >= 0; i--)
			gdk_colormap_alloc_color (cmap, &colors[i], FALSE, TRUE);
	}
#endif
}

/* maps XChat 2.0.x colors to current */
static const int remap[] =
{
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	33,	/* 16:marktextback */
	32,	/* 17:marktextfore */
	34,	/* 18: fg */
	35,	/* 19: bg */
	37,	/* 20: newdata */
	38,	/* 21: blue */
	39,	/* 22: newmsg */
	40		/* 23: away */
};

void
palette_load (void)
{
	int i, j, l, fh, res;
	char prefname[256];
	struct stat st;
	char *cfg;
	guint16 red, green, blue;
	int upgrade = FALSE;

	fh = pchat_open_file ("colors.conf", O_RDONLY, 0, 0);
	if (fh == -1)
	{
		fh = pchat_open_file ("palette.conf", O_RDONLY, 0, 0);
		upgrade = TRUE;
	}

	if (fh != -1)
	{
		fstat (fh, &st);
		cfg = malloc (st.st_size + 1);
		if (cfg)
		{
			cfg[0] = '\0';
			l = read (fh, cfg, st.st_size);
			if (l >= 0)
				cfg[l] = '\0';

			if (!upgrade)
			{
				/* mIRC colors 0-31 are here */
				for (i = 0; i < 32; i++)
				{
					snprintf (prefname, sizeof prefname, "color_%d", i);
					cfg_get_color (cfg, prefname, &red, &green, &blue);
					colors[i].red = red / 65535.0;
					colors[i].green = green / 65535.0;
					colors[i].blue = blue / 65535.0;
					colors[i].alpha = 1.0;
				}

				/* our special colors are mapped at 256+ */
				for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
				{
					snprintf (prefname, sizeof prefname, "color_%d", i);
					cfg_get_color (cfg, prefname, &red, &green, &blue);
					colors[j].red = red / 65535.0;
					colors[j].green = green / 65535.0;
					colors[j].blue = blue / 65535.0;
					colors[j].alpha = 1.0;
				}

			} else
			{
				/* loading 2.0.x palette.conf */
				for (i = 0; i < MAX_COL+1; i++)
				{
					snprintf (prefname, sizeof prefname, "color_%d_red", i);
					red = cfg_get_int (cfg, prefname);

					snprintf (prefname, sizeof prefname, "color_%d_grn", i);
					green = cfg_get_int (cfg, prefname);

					snprintf (prefname, sizeof prefname, "color_%d_blu", i);
					blue = cfg_get_int_with_result (cfg, prefname, &res);

					if (res)
					{
						colors[remap[i]].red = red / 65535.0;
						colors[remap[i]].green = green / 65535.0;
						colors[remap[i]].blue = blue / 65535.0;
						colors[remap[i]].alpha = 1.0;
					}
				}

				/* copy 0-15 to 16-31 */
				for (i = 0; i < 16; i++)
				{
					colors[i+16] = colors[i];
				}
			}
			free (cfg);
		}
		close (fh);
	}
}

void
palette_save (void)
{
	int i, j, fh;
	char prefname[256];

	fh = pchat_open_file ("colors.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		/* mIRC colors 0-31 are here */
		for (i = 0; i < 32; i++)
		{
			snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, (int)(colors[i].red * 65535), (int)(colors[i].green * 65535), (int)(colors[i].blue * 65535), prefname);
		}

		/* our special colors are mapped at 256+ */
		for (i = 256, j = 32; j < MAX_COL+1; i++, j++)
		{
			snprintf (prefname, sizeof prefname, "color_%d", i);
			cfg_put_color (fh, (int)(colors[j].red * 65535), (int)(colors[j].green * 65535), (int)(colors[j].blue * 65535), prefname);
		}

		close (fh);
	}
}
