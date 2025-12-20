/* PChat - IRC Client using GtkTextView
 * Copyright (C) 2025 Zach Bacon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "textview-chat.h"
#include "../common/xchat.h"
#include "../common/util.h"

/* IRC color/format codes */
#define IRC_BOLD        '\002'
#define IRC_COLOR       '\003'
#define IRC_RESET       '\017'
#define IRC_REVERSE     '\026'
#define IRC_ITALIC      '\035'
#define IRC_UNDERLINE   '\037'

/* Private structure */
struct _PchatTextViewChatPrivate
{
	PchatChatBuffer *current_buffer;  /* Active buffer being displayed */
	
	/* Shared tag table for all buffers */
	GtkTextTagTable *tag_table;
	
	GtkTextTag *url_tag;
	GtkTextTag *marker_tag;           /* Tag for marker line */
	
	/* Text tags for formatting */
	GtkTextTag *bold_tag;
	GtkTextTag *italic_tag;
	GtkTextTag *underline_tag;
	
	/* Color tags (indexed by mIRC color number) */
	GtkTextTag *fg_color_tags[16];
	GtkTextTag *bg_color_tags[16];
	
	/* Configuration */
	GdkRGBA palette[37];
	gint max_lines;
	gint max_auto_indent;
	gboolean show_timestamps;
	gboolean indent;
	gboolean auto_indent;
	gboolean show_separator;
	gboolean thin_separator;
	gboolean wordwrap;
	gchar *font_name;
	
	/* URL checking */
	PchatUrlCheckFunc urlcheck_func;
	
	/* Line counter */
	gint line_count;
};

G_DEFINE_TYPE_WITH_PRIVATE (PchatTextViewChat, pchat_textview_chat, GTK_TYPE_TEXT_VIEW)

enum {
	WORD_CLICKED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Standard mIRC colors */
static const GdkRGBA mirc_colors[16] = {
	{ 1.0, 1.0, 1.0, 1.0 },    /* 0 white */
	{ 0.0, 0.0, 0.0, 1.0 },    /* 1 black */
	{ 0.0, 0.0, 0.5, 1.0 },    /* 2 blue */
	{ 0.0, 0.5, 0.0, 1.0 },    /* 3 green */
	{ 1.0, 0.0, 0.0, 1.0 },    /* 4 red */
	{ 0.5, 0.0, 0.0, 1.0 },    /* 5 brown */
	{ 0.5, 0.0, 0.5, 1.0 },    /* 6 purple */
	{ 1.0, 0.5, 0.0, 1.0 },    /* 7 orange */
	{ 1.0, 1.0, 0.0, 1.0 },    /* 8 yellow */
	{ 0.0, 1.0, 0.0, 1.0 },    /* 9 light green */
	{ 0.0, 0.5, 0.5, 1.0 },    /* 10 cyan */
	{ 0.0, 1.0, 1.0, 1.0 },    /* 11 light cyan */
	{ 0.0, 0.0, 1.0, 1.0 },    /* 12 light blue */
	{ 1.0, 0.0, 1.0, 1.0 },    /* 13 pink */
	{ 0.5, 0.5, 0.5, 1.0 },    /* 14 grey */
	{ 0.75, 0.75, 0.75, 1.0 }  /* 15 light grey */
};

static void
pchat_textview_chat_create_tags (PchatTextViewChat *chat)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	gchar tag_name[32];
	gint i;
	
	/* Create shared tag table if not already created */
	if (priv->tag_table)
		return;
	
	priv->tag_table = gtk_text_tag_table_new ();
	
	/* Create formatting tags */
	priv->bold_tag = gtk_text_tag_new ("bold");
	g_object_set (priv->bold_tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_tag_table_add (priv->tag_table, priv->bold_tag);
	
	priv->italic_tag = gtk_text_tag_new ("italic");
	g_object_set (priv->italic_tag, "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_tag_table_add (priv->tag_table, priv->italic_tag);
	
	priv->underline_tag = gtk_text_tag_new ("underline");
	g_object_set (priv->underline_tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_tag_table_add (priv->tag_table, priv->underline_tag);
	
	/* Create URL tag */
	priv->url_tag = gtk_text_tag_new ("url");
	g_object_set (priv->url_tag, 
	              "foreground", "#0066cc",
	              "underline", PANGO_UNDERLINE_SINGLE,
	              "weight", PANGO_WEIGHT_NORMAL,
	              NULL);
	gtk_text_tag_table_add (priv->tag_table, priv->url_tag);
	
	/* Create marker line tag */
	priv->marker_tag = gtk_text_tag_new ("marker");
	g_object_set (priv->marker_tag,
	              "foreground", "#FF0000",
	              "weight", PANGO_WEIGHT_BOLD,
	              NULL);
	gtk_text_tag_table_add (priv->tag_table, priv->marker_tag);
	
	/* Create foreground color tags */
	for (i = 0; i < 16; i++)
	{
		g_snprintf (tag_name, sizeof(tag_name), "fg-color-%d", i);
		priv->fg_color_tags[i] = gtk_text_tag_new (tag_name);
		g_object_set (priv->fg_color_tags[i], "foreground-rgba", &mirc_colors[i], NULL);
		gtk_text_tag_table_add (priv->tag_table, priv->fg_color_tags[i]);
	}
	
	/* Create background color tags */
	for (i = 0; i < 16; i++)
	{
		g_snprintf (tag_name, sizeof(tag_name), "bg-color-%d", i);
		priv->bg_color_tags[i] = gtk_text_tag_new (tag_name);
		g_object_set (priv->bg_color_tags[i], "background-rgba", &mirc_colors[i], NULL);
		gtk_text_tag_table_add (priv->tag_table, priv->bg_color_tags[i]);
	}
}

static gboolean
pchat_textview_chat_event_after (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	PchatTextViewChat *chat = PCHAT_TEXTVIEW_CHAT (widget);
	GtkTextIter start, end, iter;
	GtkTextBuffer *buffer;
	GdkEventButton *button_event;
	gint x, y;
	GSList *tags, *tagp;
	gchar *url = NULL;
	
	if (event->type != GDK_BUTTON_RELEASE)
		return FALSE;
		
	button_event = (GdkEventButton *) event;
	
	if (button_event->button != 1)
		return FALSE;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	
	/* Get iter at mouse position */
	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget),
	                                        GTK_TEXT_WINDOW_WIDGET,
	                                        button_event->x, button_event->y,
	                                        &x, &y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);
	
	/* Check if URL tag is present */
	tags = gtk_text_iter_get_tags (&iter);
	for (tagp = tags; tagp != NULL; tagp = tagp->next)
	{
		GtkTextTag *tag = tagp->data;
		
		if (tag == chat->priv->url_tag)
		{
			/* Get the URL text */
			start = iter;
			end = iter;
			
			if (!gtk_text_iter_starts_tag (&start, tag))
				gtk_text_iter_backward_to_tag_toggle (&start, tag);
			if (!gtk_text_iter_ends_tag (&end, tag))
				gtk_text_iter_forward_to_tag_toggle (&end, tag);
			
			url = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			break;
		}
	}
	
	if (tags)
		g_slist_free (tags);
	
	if (url)
	{
		g_signal_emit (chat, signals[WORD_CLICKED], 0, url, button_event);
		g_free (url);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
pchat_textview_chat_motion_notify (GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	PchatTextViewChat *chat = PCHAT_TEXTVIEW_CHAT (widget);
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GSList *tags = NULL, *tagp = NULL;
	gint x, y;
	gboolean over_url = FALSE;
	GdkWindow *window;
	GdkCursor *cursor = NULL;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget),
	                                        GTK_TEXT_WINDOW_WIDGET,
	                                        event->x, event->y,
	                                        &x, &y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, x, y);
	
	/* Check if URL tag is present */
	tags = gtk_text_iter_get_tags (&iter);
	for (tagp = tags; tagp != NULL; tagp = tagp->next)
	{
		GtkTextTag *tag = tagp->data;
		
		if (tag == chat->priv->url_tag)
		{
			over_url = TRUE;
			break;
		}
	}
	
	if (tags)
		g_slist_free (tags);
	
	/* Set cursor based on whether we're over a URL */
	window = gtk_text_view_get_window (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_TEXT);
	if (over_url)
	{
		cursor = gdk_cursor_new_for_display (gdk_window_get_display (window), GDK_HAND2);
	}
	else
	{
		cursor = gdk_cursor_new_for_display (gdk_window_get_display (window), GDK_XTERM);
	}
	
	gdk_window_set_cursor (window, cursor);
	g_object_unref (cursor);
	
	return FALSE;
}

static gboolean
pchat_textview_chat_leave_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	GdkWindow *window;
	GdkCursor *cursor;
	
	/* Reset cursor to default when leaving the widget */
	window = gtk_text_view_get_window (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_TEXT);
	cursor = gdk_cursor_new_for_display (gdk_window_get_display (window), GDK_XTERM);
	gdk_window_set_cursor (window, cursor);
	g_object_unref (cursor);
	
	return FALSE;
}

static void
pchat_textview_chat_finalize (GObject *object)
{
	PchatTextViewChat *chat = PCHAT_TEXTVIEW_CHAT (object);
	PchatTextViewChatPrivate *priv = chat->priv;
	
	g_free (priv->font_name);
	
	G_OBJECT_CLASS (pchat_textview_chat_parent_class)->finalize (object);
}

static void
pchat_textview_chat_class_init (PchatTextViewChatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = pchat_textview_chat_finalize;
	
	signals[WORD_CLICKED] = g_signal_new ("word-clicked",
	                                       G_TYPE_FROM_CLASS (klass),
	                                       G_SIGNAL_RUN_LAST,
	                                       G_STRUCT_OFFSET (PchatTextViewChatClass, word_clicked),
	                                       NULL, NULL,
	                                       g_cclosure_marshal_VOID__STRING,
	                                       G_TYPE_NONE, 2,
	                                       G_TYPE_STRING,
	                                       GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
pchat_textview_chat_init (PchatTextViewChat *chat)
{
	PchatTextViewChatPrivate *priv;
	
	chat->priv = pchat_textview_chat_get_instance_private (chat);
	priv = chat->priv;
	
	/* No buffer initially - will be set via pchat_chat_buffer_show */
	priv->current_buffer = NULL;
	priv->tag_table = NULL;
	
	/* Create shared tag table for all buffers */
	pchat_textview_chat_create_tags (chat);
	
	/* Default settings */
	priv->max_lines = 0;
	priv->max_auto_indent = 350;
	priv->show_timestamps = TRUE;
	priv->indent = TRUE;
	priv->auto_indent = FALSE;
	priv->show_separator = TRUE;
	priv->thin_separator = FALSE;
	priv->wordwrap = TRUE;
	priv->urlcheck_func = NULL;
	
	/* Setup text view properties */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (chat), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (chat), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (chat), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (chat), 4);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (chat), 4);
	
	/* Connect click handler for URLs */
	g_signal_connect (chat, "event-after",
	                  G_CALLBACK (pchat_textview_chat_event_after), NULL);
	g_signal_connect (chat, "motion-notify-event",
	                  G_CALLBACK (pchat_textview_chat_motion_notify), NULL);
	g_signal_connect (chat, "leave-notify-event",
	                  G_CALLBACK (pchat_textview_chat_leave_notify), NULL);
	
	/* Enable motion events */
	gtk_widget_add_events (GTK_WIDGET (chat), GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
}

GtkWidget *
pchat_textview_chat_new (void)
{
	return g_object_new (PCHAT_TYPE_TEXTVIEW_CHAT, NULL);
}

/* Buffer management functions */

PchatChatBuffer *
pchat_chat_buffer_new (PchatTextViewChat *chat)
{
	PchatChatBuffer *buf;
	GtkTextIter iter;
	PchatTextViewChatPrivate *priv = chat->priv;
	
	buf = g_new0 (PchatChatBuffer, 1);
	
	/* Use shared tag table for all buffers */
	buf->buffer = gtk_text_buffer_new (priv->tag_table);
	buf->line_count = 0;
	buf->indent = 0;
	buf->marker_seen = FALSE;
	buf->show_marker = FALSE;
	buf->user_data = NULL;
	buf->marker_mark = NULL;
	buf->search_re = NULL;
	buf->search_text = NULL;
	buf->search_nee = NULL;
	buf->search_lnee = 0;
	
	/* Create end mark */
	gtk_text_buffer_get_end_iter (buf->buffer, &iter);
	buf->end_mark = gtk_text_buffer_create_mark (buf->buffer, "end", &iter, FALSE);
	
	return buf;
}

void
pchat_chat_buffer_free (PchatChatBuffer *buf)
{
	if (!buf)
		return;
	
	if (buf->search_re)
		g_regex_unref (buf->search_re);
	g_free (buf->search_text);
	g_free (buf->search_nee);
	
	/* The buffer should already be detached from any widget before freeing.
	 * We just need to release our reference. */
	if (buf->buffer)
		g_object_unref (buf->buffer);
	
	g_free (buf);
}

void
pchat_chat_buffer_show (PchatTextViewChat *chat, PchatChatBuffer *buf)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	
	if (!buf)
		return;
	
	priv->current_buffer = buf;
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (chat), buf->buffer);
	
	/* Scroll to end */
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (chat), buf->end_mark);
}

PchatChatBuffer *
pchat_textview_chat_get_buffer (PchatTextViewChat *chat)
{
	return chat->priv->current_buffer;
}

/* Marker line functions */

void
pchat_chat_buffer_set_marker (PchatChatBuffer *buf, PchatTextViewChat *chat)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	GtkTextIter iter;
	
	if (!buf)
		return;
	
	/* Remove old marker if present */
	if (buf->marker_mark)
	{
		gtk_text_buffer_get_iter_at_mark (buf->buffer, &iter, buf->marker_mark);
		gtk_text_buffer_delete_mark (buf->buffer, buf->marker_mark);
	}
	
	/* Create new marker at end */
	gtk_text_buffer_get_end_iter (buf->buffer, &iter);
	buf->marker_mark = gtk_text_buffer_create_mark (buf->buffer, "marker", &iter, TRUE);
	buf->marker_seen = FALSE;
	buf->show_marker = TRUE;
	
	/* Insert marker line */
	gtk_text_buffer_insert_with_tags (buf->buffer, &iter, 
	                                   "--- New Messages ---\n", -1,
	                                   priv->marker_tag, NULL);
}

void
pchat_chat_buffer_reset_marker (PchatChatBuffer *buf)
{
	if (!buf)
		return;
	
	buf->marker_seen = TRUE;
	buf->show_marker = FALSE;
}

gboolean
pchat_chat_buffer_marker_seen (PchatChatBuffer *buf)
{
	return buf ? buf->marker_seen : TRUE;
}

/* Helper to flush accumulated text with current formatting */
static void
flush_text_with_formatting (GtkTextBuffer *buffer, GtkTextIter *iter, GString *text,
                             PchatTextViewChatPrivate *priv, GtkWidget *widget,
                             gboolean bold, gboolean italic, gboolean underline,
                             gint fg_color, gint bg_color)
{
	GtkTextMark *start_mark;
	GtkTextIter start_iter;
	gchar *p, *word_start;
	
	if (text->len == 0)
		return;
	
	/* Mark position before insert */
	start_mark = gtk_text_buffer_create_mark (buffer, NULL, iter, TRUE);
	
	/* Insert text */
	gtk_text_buffer_insert (buffer, iter, text->str, text->len);
	
	/* Get start position */
	gtk_text_buffer_get_iter_at_mark (buffer, &start_iter, start_mark);
	
	/* Apply formatting tags */
	if (bold)
		gtk_text_buffer_apply_tag (buffer, priv->bold_tag, &start_iter, iter);
	if (italic)
		gtk_text_buffer_apply_tag (buffer, priv->italic_tag, &start_iter, iter);
	if (underline)
		gtk_text_buffer_apply_tag (buffer, priv->underline_tag, &start_iter, iter);
	if (fg_color >= 0 && fg_color < 16)
		gtk_text_buffer_apply_tag (buffer, priv->fg_color_tags[fg_color], &start_iter, iter);
	if (bg_color >= 0 && bg_color < 16)
		gtk_text_buffer_apply_tag (buffer, priv->bg_color_tags[bg_color], &start_iter, iter);
	
	/* Check for URLs on word boundaries */
	if (priv->urlcheck_func)
	{
		GtkTextIter word_start_iter, word_end_iter;
		p = text->str;
		word_start = p;
		
		while (*p)
		{
			if (isspace (*p))
			{
				if (p > word_start)
				{
					gchar *word = g_strndup (word_start, p - word_start);
					if (priv->urlcheck_func (widget, word))
					{
						/* Apply URL tag to this word */
						word_start_iter = start_iter;
						gtk_text_iter_forward_chars (&word_start_iter, word_start - text->str);
						word_end_iter = word_start_iter;
						gtk_text_iter_forward_chars (&word_end_iter, p - word_start);
						gtk_text_buffer_apply_tag (buffer, priv->url_tag, &word_start_iter, &word_end_iter);
					}
					g_free (word);
				}
				word_start = p + 1;
			}
			p++;
		}
		
		/* Check last word */
		if (p > word_start)
		{
			gchar *word = g_strndup (word_start, p - word_start);
			if (priv->urlcheck_func (widget, word))
			{
				word_start_iter = start_iter;
				gtk_text_iter_forward_chars (&word_start_iter, word_start - text->str);
				word_end_iter = word_start_iter;
				gtk_text_iter_forward_chars (&word_end_iter, p - word_start);
				gtk_text_buffer_apply_tag (buffer, priv->url_tag, &word_start_iter, &word_end_iter);
			}
			g_free (word);
		}
	}
	
	gtk_text_buffer_delete_mark (buffer, start_mark);
	g_string_truncate (text, 0);
}

/* Parse IRC color codes and apply formatting */
static void
pchat_textview_chat_append_with_formatting (PchatTextViewChat *chat, GtkTextBuffer *buffer, const gchar *text, gsize len)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	GtkTextIter iter;
	const gchar *p = text;
	const gchar *end = text + len;
	GString *current_text = g_string_new (NULL);
	gint fg_color = -1, bg_color = -1;
	gboolean bold = FALSE, italic = FALSE, underline = FALSE;
	
	gtk_text_buffer_get_end_iter (buffer, &iter);
	
	while (p < end)
	{
		if (*p == IRC_BOLD)
		{
			/* Flush accumulated text with current formatting before toggling */
			flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
			bold = !bold;
			p++;
		}
		else if (*p == IRC_ITALIC)
		{
			flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
			italic = !italic;
			p++;
		}
		else if (*p == IRC_UNDERLINE)
		{
			flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
			underline = !underline;
			p++;
		}
		else if (*p == IRC_COLOR)
		{
			flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
			p++;
			
			/* Parse color code */
			if (p < end && isdigit (*p))
			{
				fg_color = *p++ - '0';
				if (p < end && isdigit (*p))
					fg_color = fg_color * 10 + (*p++ - '0');
				fg_color = fg_color % 16;
				
				/* Check for background color */
				if (p < end && *p == ',')
				{
					p++;
					if (p < end && isdigit (*p))
					{
						bg_color = *p++ - '0';
						if (p < end && isdigit (*p))
							bg_color = bg_color * 10 + (*p++ - '0');
						bg_color = bg_color % 16;
					}
				}
			}
			else
			{
				/* IRC_COLOR without digits = reset colors, don't display the character */
				fg_color = -1;
				bg_color = -1;
			}
		}
		else if (*p == IRC_RESET)
		{
			flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
			bold = italic = underline = FALSE;
			fg_color = bg_color = -1;
			p++;
		}
		else
		{
			/* Filter out non-printable characters except newlines */
			if (*p < 32 && *p != '\n' && *p != '\t')
			{
				/* Skip non-printable control characters */
				p++;
			}
			else
			{
				/* Regular character - accumulate it */
				g_string_append_c (current_text, *p);
				p++;
			}
		}
	}
	
	/* Flush any remaining text */
	flush_text_with_formatting (buffer, &iter, current_text, priv, GTK_WIDGET (chat), bold, italic, underline, fg_color, bg_color);
	g_string_free (current_text, TRUE);
}

void
pchat_textview_chat_append (PchatTextViewChat *chat, const gchar *text, gsize len)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	PchatChatBuffer *buf;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	buf = priv->current_buffer;
	if (!buf)
		return;
	
	if (len == 0)
		len = strlen (text);
	
	pchat_textview_chat_append_with_formatting (chat, buf->buffer, text, len);
	buf->line_count++;
	
	/* Auto-scroll to bottom */
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (chat), buf->end_mark);
}

/* Buffer-specific append - for appending to buffers that aren't currently shown */
void
pchat_chat_buffer_append (PchatChatBuffer *buf, PchatTextViewChat *chat,
                          const gchar *text, gsize len)
{
	if (!buf || !chat)
		return;
	
	if (len == 0)
		len = strlen (text);
	
	pchat_textview_chat_append_with_formatting (chat, buf->buffer, text, len);
	buf->line_count++;
}

void
pchat_textview_chat_append_with_stamp (PchatTextViewChat *chat, PchatChatBuffer *buf,
                                        const gchar *text, gsize len, time_t stamp)
{
	GString *full_text;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	if (!buf)
		return;
	
	full_text = g_string_new (NULL);
	
	/* Add timestamp if enabled */
	if (chat->priv->show_timestamps && stamp != 0)
	{
		struct tm *tm_ptr = localtime (&stamp);
		gchar time_str[64];
		strftime (time_str, sizeof (time_str), "%H:%M:%S ", tm_ptr);
		g_string_append (full_text, time_str);
	}
	
	/* Add text */
	if (text && len > 0)
		g_string_append_len (full_text, text, len);
	
	g_string_append_c (full_text, '\n');
	
	pchat_chat_buffer_append (buf, chat, full_text->str, full_text->len);
	g_string_free (full_text, TRUE);
}

void
pchat_textview_chat_append_indent (PchatTextViewChat *chat, PchatChatBuffer *buf,
                                    const gchar *left_text, gsize left_len,
                                    const gchar *right_text, gsize right_len,
                                    time_t stamp)
{
	GString *full_text;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	if (!buf)
		return;
	
	full_text = g_string_new (NULL);
	
	/* Add timestamp if enabled */
	if (chat->priv->show_timestamps && stamp != 0)
	{
		struct tm *tm_ptr = localtime (&stamp);
		gchar time_str[64];
		strftime (time_str, sizeof (time_str), "%H:%M:%S ", tm_ptr);
		g_string_append (full_text, time_str);
	}
	
	/* Add left text */
	if (left_text && left_len > 0)
	{
		g_string_append_len (full_text, left_text, left_len);
		if (chat->priv->indent)
			g_string_append (full_text, " ");
	}
	
	/* Add right text */
	if (right_text && right_len > 0)
		g_string_append_len (full_text, right_text, right_len);
	
	g_string_append_c (full_text, '\n');
	
	pchat_chat_buffer_append (buf, chat, full_text->str, full_text->len);
	g_string_free (full_text, TRUE);
}

void
pchat_chat_buffer_append_indent (PchatChatBuffer *buf, PchatTextViewChat *chat,
                                  const gchar *left_text, gsize left_len,
                                  const gchar *right_text, gsize right_len,
                                  time_t stamp)
{
	GString *full_text;
	
	if (!buf || !chat)
		return;
	
	full_text = g_string_new (NULL);
	
	/* Add timestamp if enabled */
	if (chat->priv->show_timestamps && stamp != 0)
	{
		struct tm *tm_ptr = localtime (&stamp);
		gchar time_str[64];
		strftime (time_str, sizeof (time_str), "%H:%M:%S ", tm_ptr);
		g_string_append (full_text, time_str);
	}
	
	/* Add left text */
	if (left_text && left_len > 0)
	{
		g_string_append_len (full_text, left_text, left_len);
		if (chat->priv->indent)
			g_string_append (full_text, " ");
	}
	
	/* Add right text */
	if (right_text && right_len > 0)
		g_string_append_len (full_text, right_text, right_len);
	
	g_string_append_c (full_text, '\n');
	
	pchat_chat_buffer_append (buf, chat, full_text->str, full_text->len);
	g_string_free (full_text, TRUE);
}

void
pchat_textview_chat_clear (PchatTextViewChat *chat, gint lines)
{
	PchatTextViewChatPrivate *priv = chat->priv;
	PchatChatBuffer *buf;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	buf = priv->current_buffer;
	if (!buf)
		return;
	
	/* If lines == 0, clear everything */
	if (lines == 0)
	{
		gtk_text_buffer_set_text (buf->buffer, "", 0);
		buf->line_count = 0;
	}
	else
	{
		/* TODO: Implement partial clearing */
	}
}

void
pchat_chat_buffer_clear (PchatChatBuffer *buf, gint lines)
{
	if (!buf)
		return;
	
	/* If lines == 0, clear everything */
	if (lines == 0)
	{
		gtk_text_buffer_set_text (buf->buffer, "", 0);
		buf->line_count = 0;
	}
	else
	{
		/* TODO: Implement partial clearing */
	}
}

void
pchat_textview_chat_set_font (PchatTextViewChat *chat, const gchar *font_name)
{
	PangoFontDescription *font_desc;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	g_free (chat->priv->font_name);
	chat->priv->font_name = g_strdup (font_name);
	
	font_desc = pango_font_description_from_string (font_name);
	
	/* Use CSS provider for font settings (GTK3.16+) */
	{
		GtkCssProvider *provider;
		GtkStyleContext *context;
		gchar *css;
		
		provider = gtk_css_provider_new ();
		css = g_strdup_printf ("textview { font: %s; }", font_name);
		gtk_css_provider_load_from_data (provider, css, -1, NULL);
		g_free (css);
		
		context = gtk_widget_get_style_context (GTK_WIDGET (chat));
		gtk_style_context_add_provider (context,
		                                 GTK_STYLE_PROVIDER (provider),
		                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_object_unref (provider);
	}
	
	pango_font_description_free (font_desc);
}

void
pchat_textview_chat_set_max_lines (PchatTextViewChat *chat, gint max_lines)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->max_lines = max_lines;
}

void
pchat_textview_chat_set_show_timestamps (PchatTextViewChat *chat, gboolean show)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->show_timestamps = show;
}

void
pchat_textview_chat_set_palette (PchatTextViewChat *chat, GdkRGBA palette[], gint palette_size)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	
	if (palette_size > 37)
		palette_size = 37;
	
	memcpy (chat->priv->palette, palette, sizeof (GdkRGBA) * palette_size);
	
	/* Apply default background and foreground colors using CSS (indices 34 and 35) */
	if (palette_size > 35)
	{
		GtkCssProvider *provider;
		GtkStyleContext *context;
		gchar *css;
		gchar *fg_hex, *bg_hex;
		
		provider = gtk_css_provider_new ();
		
		/* Convert palette colors to hex strings */
		fg_hex = g_strdup_printf ("#%02x%02x%02x", 
		                          (int)(palette[34].red * 255),
		                          (int)(palette[34].green * 255),
		                          (int)(palette[34].blue * 255));
		bg_hex = g_strdup_printf ("#%02x%02x%02x", 
		                          (int)(palette[35].red * 255),
		                          (int)(palette[35].green * 255),
		                          (int)(palette[35].blue * 255));
		
		css = g_strdup_printf ("textview text { color: %s; background-color: %s; }", fg_hex, bg_hex);
		
		g_print ("Applying CSS: %s\n", css);
		
		gtk_css_provider_load_from_data (provider, css, -1, NULL);
		
		context = gtk_widget_get_style_context (GTK_WIDGET (chat));
		gtk_style_context_add_provider (context,
		                                 GTK_STYLE_PROVIDER (provider),
		                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
		
		g_free (css);
		g_free (fg_hex);
		g_free (bg_hex);
		g_object_unref (provider);
	}
}

void
pchat_textview_chat_set_urlcheck_function (PchatTextViewChat *chat, PchatUrlCheckFunc func)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->urlcheck_func = func;
}

void
pchat_textview_chat_set_indent (PchatTextViewChat *chat, gboolean indent)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->indent = indent;
}

void
pchat_textview_chat_set_auto_indent (PchatTextViewChat *chat, gboolean auto_indent)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->auto_indent = auto_indent;
}

void
pchat_textview_chat_set_max_auto_indent (PchatTextViewChat *chat, gint max_auto_indent)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->max_auto_indent = max_auto_indent;
}

void
pchat_textview_chat_set_show_separator (PchatTextViewChat *chat, gboolean show)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->show_separator = show;
}

void
pchat_textview_chat_set_thin_separator (PchatTextViewChat *chat, gboolean thin)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->thin_separator = thin;
}

void
pchat_textview_chat_set_wordwrap (PchatTextViewChat *chat, gboolean wordwrap)
{
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	chat->priv->wordwrap = wordwrap;
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (chat),
	                              wordwrap ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
}

gboolean
pchat_textview_chat_search (PchatTextViewChat *chat, const gchar *text,
                             PchatSearchFlags flags, GError **error)
{
	PchatTextViewChatPrivate *priv;
	PchatChatBuffer *buf;
	GtkTextIter start, end, match_start, match_end;
	GtkTextSearchFlags search_flags = 0;
	gboolean found;
	
	g_return_val_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat), FALSE);
	g_return_val_if_fail (text != NULL, FALSE);
	
	priv = chat->priv;
	buf = priv->current_buffer;
	
	if (!buf)
		return FALSE;
	
	/* Set search flags */
	if (!(flags & PCHAT_SEARCH_CASE_MATCH))
		search_flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;
	if (flags & PCHAT_SEARCH_VISIBLE_ONLY)
		search_flags |= GTK_TEXT_SEARCH_VISIBLE_ONLY;
	
	/* Get search start position */
	if (flags & PCHAT_SEARCH_BACKWARD)
	{
		gtk_text_buffer_get_selection_bounds (buf->buffer, &start, NULL);
		if (!gtk_text_iter_equal (&start, &end))
			end = start;
		else
			gtk_text_buffer_get_start_iter (buf->buffer, &end);
		gtk_text_buffer_get_start_iter (buf->buffer, &start);
	}
	else
	{
		gtk_text_buffer_get_selection_bounds (buf->buffer, NULL, &start);
		if (!gtk_text_iter_equal (&start, &end))
			end = start;
		else
			gtk_text_buffer_get_end_iter (buf->buffer, &start);
		gtk_text_buffer_get_end_iter (buf->buffer, &end);
	}
	
	/* Perform search */
	if (flags & PCHAT_SEARCH_BACKWARD)
		found = gtk_text_iter_backward_search (&start, text, search_flags,
		                                        &match_start, &match_end, &end);
	else
		found = gtk_text_iter_forward_search (&start, text, search_flags,
		                                       &match_start, &match_end, &end);
	
	if (found)
	{
		/* Select the match */
		gtk_text_buffer_select_range (buf->buffer, &match_start, &match_end);
		
		/* Scroll to make it visible */
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (chat), &match_start,
		                               0.0, TRUE, 0.0, 0.5);
		
		/* Highlight if requested */
		if (flags & PCHAT_SEARCH_HIGHLIGHT)
		{
			/* TODO: Add persistent highlighting */
		}
		
		return TRUE;
	}
	
	return FALSE;
}

void
pchat_textview_chat_save (PchatTextViewChat *chat, int fd)
{
	PchatTextViewChatPrivate *priv;
	PchatChatBuffer *buf;
	GtkTextIter start, end;
	gchar *text;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	g_return_if_fail (fd >= 0);
	
	priv = chat->priv;
	buf = priv->current_buffer;
	
	if (!buf)
		return;
	
	/* Get all text from buffer */
	gtk_text_buffer_get_bounds (buf->buffer, &start, &end);
	text = gtk_text_buffer_get_text (buf->buffer, &start, &end, FALSE);
	
	if (text)
	{
		write (fd, text, strlen (text));
		g_free (text);
	}
}

void
pchat_chat_buffer_save (PchatChatBuffer *buf, int fd)
{
	GtkTextIter start, end;
	gchar *text;
	
	g_return_if_fail (buf != NULL);
	g_return_if_fail (fd >= 0);
	
	/* Get all text from buffer */
	gtk_text_buffer_get_bounds (buf->buffer, &start, &end);
	text = gtk_text_buffer_get_text (buf->buffer, &start, &end, FALSE);
	
	if (text)
	{
		write (fd, text, strlen (text));
		g_free (text);
	}
}

gboolean
pchat_chat_buffer_is_empty (PchatChatBuffer *buf)
{
	return buf ? (buf->line_count == 0) : TRUE;
}

void
pchat_textview_chat_foreach (PchatTextViewChat *chat,
                              void (*func)(PchatTextViewChat *chat, const gchar *line, gpointer data),
                              gpointer data)
{
	PchatTextViewChatPrivate *priv;
	PchatChatBuffer *buf;
	GtkTextIter start, end, line_start, line_end;
	gchar *line_text;
	gint line_count;
	
	g_return_if_fail (PCHAT_IS_TEXTVIEW_CHAT (chat));
	g_return_if_fail (func != NULL);
	
	priv = chat->priv;
	buf = priv->current_buffer;
	
	if (!buf)
		return;
	
	gtk_text_buffer_get_bounds (buf->buffer, &start, &end);
	line_count = gtk_text_buffer_get_line_count (buf->buffer);
	
	for (gint i = 0; i < line_count; i++)
	{
		gtk_text_buffer_get_iter_at_line (buf->buffer, &line_start, i);
		line_end = line_start;
		
		if (!gtk_text_iter_ends_line (&line_end))
			gtk_text_iter_forward_to_line_end (&line_end);
		
		line_text = gtk_text_buffer_get_text (buf->buffer, &line_start, &line_end, FALSE);
		
		if (line_text)
		{
			func (chat, line_text, data);
			g_free (line_text);
		}
	}
}

/* Helper function to search a line for a match */
static gboolean
pchat_chat_buffer_line_matches (PchatChatBuffer *buf, const gchar *line_text)
{
	if (!buf || !line_text)
		return FALSE;
	
	/* If we have a regex, use that */
	if (buf->search_re)
	{
		return g_regex_match (buf->search_re, line_text, 0, NULL);
	}
	/* Otherwise use plain text search */
	else if (buf->search_nee)
	{
		gchar *haystack = g_utf8_casefold (line_text, -1);
		gboolean match = (strstr (haystack, buf->search_nee) != NULL);
		g_free (haystack);
		return match;
	}
	
	return FALSE;
}

/* Set up regex search for lastlog */
void
pchat_chat_buffer_set_search_regex (PchatChatBuffer *buf, const gchar *pattern,
                                     gboolean case_sensitive, GError **error)
{
	GRegexCompileFlags flags = 0;
	
	g_return_if_fail (buf != NULL);
	g_return_if_fail (pattern != NULL);
	
	/* Clear any existing search */
	pchat_chat_buffer_clear_search (buf);
	
	/* Compile regex */
	if (!case_sensitive)
		flags |= G_REGEX_CASELESS;
	
	buf->search_re = g_regex_new (pattern, flags, 0, error);
	if (buf->search_re)
		buf->search_text = g_strdup (pattern);
}

/* Set up plain text search for lastlog */
void
pchat_chat_buffer_set_search_text (PchatChatBuffer *buf, const gchar *text,
                                    gboolean case_sensitive)
{
	g_return_if_fail (buf != NULL);
	g_return_if_fail (text != NULL);
	
	/* Clear any existing search */
	pchat_chat_buffer_clear_search (buf);
	
	buf->search_text = g_strdup (text);
	
	if (case_sensitive)
	{
		buf->search_nee = g_strdup (text);
	}
	else
	{
		buf->search_nee = g_utf8_casefold (text, -1);
	}
	
	buf->search_lnee = strlen (buf->search_nee);
}

/* Clear search criteria */
void
pchat_chat_buffer_clear_search (PchatChatBuffer *buf)
{
	if (!buf)
		return;
	
	if (buf->search_re)
	{
		g_regex_unref (buf->search_re);
		buf->search_re = NULL;
	}
	
	g_free (buf->search_text);
	buf->search_text = NULL;
	
	g_free (buf->search_nee);
	buf->search_nee = NULL;
	
	buf->search_lnee = 0;
}


/* Lastlog - copy matching lines from search_area to output buffer
 * This is similar to gtk_xtext_lastlog but works with GtkTextBuffer */
gint
pchat_chat_buffer_lastlog (PchatChatBuffer *output, PchatChatBuffer *search_area,
                            PchatTextViewChat *chat)
{
	GtkTextIter line_start, line_end;
	gchar *line_text;
	gint line_count, matches = 0;
	
	g_return_val_if_fail (output != NULL, 0);
	g_return_val_if_fail (search_area != NULL, 0);
	g_return_val_if_fail (chat != NULL, 0);
	
	/* Check if search_area has search criteria set */
	if (!search_area->search_re && !search_area->search_nee)
		return 0;
	
	line_count = gtk_text_buffer_get_line_count (search_area->buffer);
	
	/* Iterate through all lines in search_area */
	for (gint i = 0; i < line_count; i++)
	{
		gtk_text_buffer_get_iter_at_line (search_area->buffer, &line_start, i);
		line_end = line_start;
		
		if (!gtk_text_iter_ends_line (&line_end))
			gtk_text_iter_forward_to_line_end (&line_end);
		
		line_text = gtk_text_buffer_get_text (search_area->buffer, &line_start, &line_end, FALSE);
		
		if (line_text)
		{
			/* Check if this line matches the search criteria */
			if (pchat_chat_buffer_line_matches (search_area, line_text))
			{
				/* Copy the line to output buffer */
				GtkTextIter output_end;
				gtk_text_buffer_get_end_iter (output->buffer, &output_end);
				gtk_text_buffer_insert (output->buffer, &output_end, line_text, -1);
				gtk_text_buffer_insert (output->buffer, &output_end, "\n", 1);
				
				output->line_count++;
				matches++;
			}
			g_free (line_text);
		}
	}
	
	return matches;
}


