/* PChat Audio Player Plugin - GUI Interface
 * Copyright (C) 2025
 */

#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "audioplayer.h"

/* Undefine xchat macros to allow direct struct member access */
#undef xchat_printf

/* GUI state */
static GtkWidget *main_window = NULL;
static GtkWidget *playlist_view = NULL;
static GtkWidget *play_button = NULL;
static GtkWidget *pause_button = NULL;
static GtkWidget *stop_button = NULL;
static GtkWidget *prev_button = NULL;
static GtkWidget *next_button = NULL;
static GtkWidget *now_playing_label = NULL;
static GtkWidget *time_label = NULL;
static GtkWidget *volume_scale = NULL;
static GtkListStore *playlist_store = NULL;

static xchat_plugin *ph_gui = NULL;
static AudioPlayer *player_gui = NULL;
static guint update_timer = 0;

enum {
    COL_TITLE,
    COL_FILEPATH,
    COL_POINTER,
    N_COLUMNS
};

/* Forward declarations */
static void update_playlist_view(void);
static void update_now_playing(void);
static gboolean update_timer_callback(gpointer data);

/* Button callbacks */
static void on_play_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    
    if (audioplayer_get_state(player_gui) == STATE_PAUSED) {
        audioplayer_resume(player_gui);
    } else {
        /* Play first track in playlist */
        PlaylistItem *first = audioplayer_get_playlist(player_gui);
        if (first) {
            audioplayer_play(player_gui, first->filepath);
        }
    }
}

static void on_pause_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    audioplayer_pause(player_gui);
}

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    audioplayer_stop(player_gui);
}

static void on_prev_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    audioplayer_prev(player_gui);
}

static void on_next_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    audioplayer_next(player_gui);
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double value = gtk_range_get_value(range);
    audioplayer_set_volume(player_gui, (float)value / 100.0f);
}

static void on_add_file_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Add Audio File",
        GTK_WINDOW(main_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    
    /* Add audio file filter */
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio Files");
    gtk_file_filter_add_pattern(filter, "*.mp3");
    gtk_file_filter_add_pattern(filter, "*.flac");
    gtk_file_filter_add_pattern(filter, "*.ogg");
    gtk_file_filter_add_pattern(filter, "*.wav");
    gtk_file_filter_add_pattern(filter, "*.m4a");
    gtk_file_filter_add_pattern(filter, "*.opus");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All Files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        GSList *iter = filenames;
        
        while (iter) {
            char *filename = (char *)iter->data;
            audioplayer_add_to_playlist(player_gui, filename);
            g_free(filename);
            iter = iter->next;
        }
        
        g_slist_free(filenames);
        update_playlist_view();
    }
    
    gtk_widget_destroy(dialog);
}

static void on_load_playlist_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Load Playlist",
        GTK_WINDOW(main_window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);
    
    /* Add playlist file filter */
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Playlist Files");
    gtk_file_filter_add_pattern(filter, "*.m3u");
    gtk_file_filter_add_pattern(filter, "*.m3u8");
    gtk_file_filter_add_pattern(filter, "*.pls");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        int count = audioplayer_load_playlist_file(player_gui, filename);
        
        if (count > 0) {
            ph_gui->xchat_printf(ph_gui, "[AudioPlayer] Loaded %d tracks from playlist\n", count);
        } else if (count == 0) {
            ph_gui->xchat_printf(ph_gui, "[AudioPlayer] No valid tracks found in playlist\n");
        } else {
            ph_gui->xchat_printf(ph_gui, "[AudioPlayer] Failed to load playlist\n");
        }
        
        g_free(filename);
        update_playlist_view();
    }
    
    gtk_widget_destroy(dialog);
}

static void on_clear_playlist_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    
    audioplayer_clear_playlist(player_gui);
    update_playlist_view();
}

static void on_playlist_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                     GtkTreeViewColumn *column, gpointer user_data) {
    (void)column;
    (void)user_data;
    
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_store), &iter, path)) {
        gchar *filepath;
        gtk_tree_model_get(GTK_TREE_MODEL(playlist_store), &iter, COL_FILEPATH, &filepath, -1);
        
        if (filepath) {
            audioplayer_play(player_gui, filepath);
            g_free(filepath);
        }
    }
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    
    if (update_timer) {
        g_source_remove(update_timer);
        update_timer = 0;
    }
    
    main_window = NULL;
}

static void update_playlist_view(void) {
    if (!playlist_store) return;
    
    gtk_list_store_clear(playlist_store);
    
    PlaylistItem *item = audioplayer_get_playlist(player_gui);
    PlaylistItem *current = audioplayer_get_current_track(player_gui);
    
    while (item) {
        GtkTreeIter iter;
        gtk_list_store_append(playlist_store, &iter);
        
        /* Format: [▶] Artist - Title */
        gchar *display_title;
        if (item->artist && item->title) {
            display_title = g_strdup_printf("%s%s - %s",
                (item == current) ? "▶ " : "",
                item->artist,
                item->title);
        } else {
            display_title = g_strdup_printf("%s%s",
                (item == current) ? "▶ " : "",
                item->title);
        }
        
        gtk_list_store_set(playlist_store, &iter,
            COL_TITLE, display_title,
            COL_FILEPATH, item->filepath,
            COL_POINTER, item,
            -1);
        
        g_free(display_title);
        item = item->next;
    }
}

static void update_now_playing(void) {
    if (!now_playing_label) return;
    
    PlaylistItem *current = audioplayer_get_current_track(player_gui);
    PlayerState state = audioplayer_get_state(player_gui);
    
    if (current && state != STATE_STOPPED) {
        const char *state_str = "";
        switch (state) {
            case STATE_PLAYING: state_str = "Playing"; break;
            case STATE_PAUSED: state_str = "Paused"; break;
            default: state_str = "Stopped"; break;
        }
        
        gchar *text;
        if (current->artist && current->title) {
            /* Show "Artist - Title [Album]" if metadata available */
            if (current->album) {
                text = g_strdup_printf("<b>%s:</b> %s - %s <i>[%s]</i>",
                    state_str, current->artist, current->title, current->album);
            } else {
                text = g_strdup_printf("<b>%s:</b> %s - %s",
                    state_str, current->artist, current->title);
            }
        } else {
            text = g_strdup_printf("<b>%s:</b> %s", state_str, current->title);
        }
        
        gtk_label_set_markup(GTK_LABEL(now_playing_label), text);
        g_free(text);
    } else {
        gtk_label_set_text(GTK_LABEL(now_playing_label), "Not playing");
    }
}

static gboolean update_timer_callback(gpointer data) {
    (void)data;
    
    update_now_playing();
    update_playlist_view();
    
    return TRUE;  /* Continue timer */
}

void audioplayer_gui_init(xchat_plugin *ph, AudioPlayer *player) {
    ph_gui = ph;
    player_gui = player;
    
    if (main_window) {
        /* Window already exists, just show it */
        gtk_window_present(GTK_WINDOW(main_window));
        return;
    }
    
    /* Create main window */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Audio Player");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 10);
    g_signal_connect(main_window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    /* Create main vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);
    
    /* Now playing label */
    now_playing_label = gtk_label_new("Not playing");
    gtk_label_set_xalign(GTK_LABEL(now_playing_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), now_playing_label, FALSE, FALSE, 5);
    
    /* Playback controls */
    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), controls_box, FALSE, FALSE, 5);
    
    prev_button = gtk_button_new_with_label("⏮ Previous");
    play_button = gtk_button_new_with_label("▶ Play");
    pause_button = gtk_button_new_with_label("⏸ Pause");
    stop_button = gtk_button_new_with_label("⏹ Stop");
    next_button = gtk_button_new_with_label("⏭ Next");
    
    gtk_box_pack_start(GTK_BOX(controls_box), prev_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), play_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), pause_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), stop_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), next_button, TRUE, TRUE, 0);
    
    g_signal_connect(prev_button, "clicked", G_CALLBACK(on_prev_clicked), NULL);
    g_signal_connect(play_button, "clicked", G_CALLBACK(on_play_clicked), NULL);
    g_signal_connect(pause_button, "clicked", G_CALLBACK(on_pause_clicked), NULL);
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_clicked), NULL);
    
    /* Volume control */
    GtkWidget *volume_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), volume_box, FALSE, FALSE, 5);
    
    GtkWidget *volume_label = gtk_label_new("🔊 Volume:");
    gtk_box_pack_start(GTK_BOX(volume_box), volume_label, FALSE, FALSE, 0);
    
    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    gtk_scale_set_value_pos(GTK_SCALE(volume_scale), GTK_POS_RIGHT);
    gtk_range_set_value(GTK_RANGE(volume_scale), audioplayer_get_volume(player_gui) * 100.0);
    gtk_box_pack_start(GTK_BOX(volume_box), volume_scale, TRUE, TRUE, 0);
    
    g_signal_connect(volume_scale, "value-changed", G_CALLBACK(on_volume_changed), NULL);
    
    /* Playlist controls */
    GtkWidget *playlist_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), playlist_controls, FALSE, FALSE, 5);
    
    GtkWidget *add_file_btn = gtk_button_new_with_label("➕ Add Files");
    GtkWidget *load_playlist_btn = gtk_button_new_with_label("📂 Load Playlist");
    GtkWidget *clear_btn = gtk_button_new_with_label("🗑 Clear");
    
    gtk_box_pack_start(GTK_BOX(playlist_controls), add_file_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(playlist_controls), load_playlist_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(playlist_controls), clear_btn, TRUE, TRUE, 0);
    
    g_signal_connect(add_file_btn, "clicked", G_CALLBACK(on_add_file_clicked), NULL);
    g_signal_connect(load_playlist_btn, "clicked", G_CALLBACK(on_load_playlist_clicked), NULL);
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_playlist_clicked), NULL);
    
    /* Playlist view */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    playlist_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    playlist_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist_store));
    gtk_container_add(GTK_CONTAINER(scrolled), playlist_view);
    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "Playlist", renderer, "text", COL_TITLE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_view), column);
    
    g_signal_connect(playlist_view, "row-activated",
        G_CALLBACK(on_playlist_row_activated), NULL);
    
    /* Initial update */
    update_playlist_view();
    update_now_playing();
    
    /* Start update timer (every 500ms) */
    update_timer = g_timeout_add(500, update_timer_callback, NULL);
    
    /* Show window */
    gtk_widget_show_all(main_window);
}

void audioplayer_gui_update(void) {
    if (main_window) {
        update_now_playing();
        update_playlist_view();
    }
}

void audioplayer_gui_cleanup(void) {
    if (update_timer) {
        g_source_remove(update_timer);
        update_timer = 0;
    }
    
    if (main_window) {
        gtk_widget_destroy(main_window);
        main_window = NULL;
    }
}
