/* PChat Audio Player Plugin - PChat Integration
 * Copyright (C) 2025
 *
 * This is the main plugin file that interfaces with PChat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <strings.h>
#include "pchat-plugin.h"
#include "audioplayer.h"
#include "gui.h"

/* Undefine pchat macros to allow direct struct member access */
#undef pchat_hook_command
#undef pchat_print
#undef pchat_printf
#undef pchat_command
#undef pchat_commandf

#define PNAME "AudioPlayer"
#define PDESC "Audio player with FFmpeg decoding and FAudio output"
#define PVERSION "1.0"

static pchat_plugin *ph;   /* plugin handle */
static AudioPlayer *player = NULL;

/* Command callbacks */
static int play_cb(char *word[], char *word_eol[], void *userdata);
static int pause_cb(char *word[], char *word_eol[], void *userdata);
static int stop_cb(char *word[], char *word_eol[], void *userdata);
static int next_cb(char *word[], char *word_eol[], void *userdata);
static int prev_cb(char *word[], char *word_eol[], void *userdata);
static int playlist_cb(char *word[], char *word_eol[], void *userdata);
static int nowplaying_cb(char *word[], char *word_eol[], void *userdata);
static int gui_cb(char *word[], char *word_eol[], void *userdata);

/* Helper function to print formatted messages */
static void print_msg(const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ph->pchat_printf(ph, "[AudioPlayer] %s\n", buffer);
}

/* Unescape shell escape sequences in paths */
static char* unescape_path(const char *path) {
    char *result = malloc(strlen(path) + 1);
    if (!result) return NULL;
    
    const char *src = path;
    char *dst = result;
    
    while (*src) {
        /* Handle '\'' (escaped single quote in shell) */
        if (src[0] == '\'' && src[1] == '\\' && src[2] == '\'' && src[3] == '\'') {
            *dst++ = '\'';
            src += 4;
        }
        /* Handle \" (escaped double quote) */
        else if (src[0] == '\\' && src[1] == '"') {
            *dst++ = '"';
            src += 2;
        }
        /* Handle \' (escaped single quote) */
        else if (src[0] == '\\' && src[1] == '\'') {
            *dst++ = '\'';
            src += 2;
        }
        /* Handle \\ (escaped backslash) */
        else if (src[0] == '\\' && src[1] == '\\') {
            *dst++ = '\\';
            src += 2;
        }
        /* Handle other backslash escapes */
        else if (src[0] == '\\' && src[1]) {
            *dst++ = src[1];
            src += 2;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return result;
}

/* /PLAY command - play a file or URL */
static int play_cb(char *word[], char *word_eol[], void *userdata) {
    (void)userdata;
    
    if (!word[2] || !*word[2]) {
        /* Resume if paused */
        if (audioplayer_get_state(player) == STATE_PAUSED) {
            if (audioplayer_resume(player) == 0) {
                print_msg("Resumed playback");
            } else {
                print_msg("Failed to resume playback");
            }
        } else {
            print_msg("Usage: /PLAY <filepath> - Play an audio file");
            print_msg("       /PLAY - Resume playback if paused");
        }
        return PCHAT_EAT_ALL;
    }
    
    /* Use word_eol[2] to get the full path including spaces */
    const char *filepath = word_eol[2];
    
    /* Trim leading/trailing whitespace and quotes */
    while (*filepath == ' ' || *filepath == '\t') filepath++;
    
    char *filepath_copy = strdup(filepath);
    char *end = filepath_copy + strlen(filepath_copy) - 1;
    while (end > filepath_copy && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    /* Remove surrounding quotes if present */
    char *clean_path = filepath_copy;
    if (*clean_path == '"' || *clean_path == '\'') {
        clean_path++;
        end = clean_path + strlen(clean_path) - 1;
        if (*end == '"' || *end == '\'') {
            *end = '\0';
        }
    }
    
    /* Unescape shell escape sequences */
    char *unescaped = unescape_path(clean_path);
    
    if (audioplayer_play(player, unescaped) == 0) {
        print_msg("Now playing: %s", unescaped);
    } else {
        print_msg("Failed to play: %s", unescaped);
    }
    
    free(unescaped);
    free(filepath_copy);
    return PCHAT_EAT_ALL;
}

/* /PAUSE command - pause playback */
static int pause_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word;
    (void)word_eol;
    (void)userdata;
    
    PlayerState state = audioplayer_get_state(player);
    
    if (state == STATE_PLAYING) {
        if (audioplayer_pause(player) == 0) {
            print_msg("Paused playback");
        } else {
            print_msg("Failed to pause playback");
        }
    } else if (state == STATE_PAUSED) {
        if (audioplayer_resume(player) == 0) {
            print_msg("Resumed playback");
        } else {
            print_msg("Failed to resume playback");
        }
    } else {
        print_msg("Nothing is playing");
    }
    
    return PCHAT_EAT_ALL;
}

/* /STOP command - stop playback */
static int stop_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word;
    (void)word_eol;
    (void)userdata;
    
    if (audioplayer_stop(player) == 0) {
        print_msg("Stopped playback");
    } else {
        print_msg("Failed to stop playback");
    }
    
    return PCHAT_EAT_ALL;
}

/* /NEXT command - play next track */
static int next_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word;
    (void)word_eol;
    (void)userdata;
    
    if (audioplayer_next(player) == 0) {
        PlaylistItem *track = audioplayer_get_current_track(player);
        if (track) {
            print_msg("Playing next: %s", track->title);
        }
    } else {
        print_msg("No next track available");
    }
    
    return PCHAT_EAT_ALL;
}

/* /PREV command - play previous track */
static int prev_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word;
    (void)word_eol;
    (void)userdata;
    
    if (audioplayer_prev(player) == 0) {
        PlaylistItem *track = audioplayer_get_current_track(player);
        if (track) {
            print_msg("Playing previous: %s", track->title);
        }
    } else {
        print_msg("No previous track available");
    }
    
    return PCHAT_EAT_ALL;
}

/* /PLAYLIST command - manage playlist */
static int playlist_cb(char *word[], char *word_eol[], void *userdata) {
    (void)userdata;
    
    if (!word[2] || !*word[2]) {
        /* List playlist */
        int count = audioplayer_get_playlist_count(player);
        if (count == 0) {
            print_msg("Playlist is empty");
        } else {
            print_msg("Playlist (%d tracks):", count);
            
            PlaylistItem *item = audioplayer_get_playlist(player);
            PlaylistItem *current = audioplayer_get_current_track(player);
            int index = 1;
            
            while (item) {
                const char *marker = (item == current) ? "► " : "  ";
                print_msg("%s%d. %s", marker, index, item->title);
                item = item->next;
                index++;
            }
        }
        return PCHAT_EAT_ALL;
    }
    
    const char *subcmd = word[2];
    
    if (strcasecmp(subcmd, "add") == 0) {
        if (!word[3] || !*word[3]) {
            print_msg("Usage: /PLAYLIST ADD <filepath>");
            return PCHAT_EAT_ALL;
        }
        
        /* Use word_eol[3] to get the full path including spaces */
        const char *filepath = word_eol[3];
        
        /* Trim leading/trailing whitespace and quotes */
        while (*filepath == ' ' || *filepath == '\t') filepath++;
        
        char *filepath_copy = strdup(filepath);
        char *end = filepath_copy + strlen(filepath_copy) - 1;
        while (end > filepath_copy && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }
        
        /* Remove surrounding quotes if present */
        char *clean_path = filepath_copy;
        if (*clean_path == '"' || *clean_path == '\'') {
            clean_path++;
            end = clean_path + strlen(clean_path) - 1;
            if (*end == '"' || *end == '\'') {
                *end = '\0';
            }
        }
        
        /* Unescape shell escape sequences */
        char *unescaped = unescape_path(clean_path);
        
        if (audioplayer_add_to_playlist(player, unescaped) == 0) {
            print_msg("Added to playlist: %s", unescaped);
        } else {
            print_msg("Failed to add to playlist: %s", unescaped);
        }
        
        free(unescaped);
        free(filepath_copy);
    }
    else if (strcasecmp(subcmd, "load") == 0) {
        if (!word[3] || !*word[3]) {
            print_msg("Usage: /PLAYLIST LOAD <playlist_file.m3u|.pls>");
            return PCHAT_EAT_ALL;
        }
        
        /* Use word_eol[3] to get the full path including spaces */
        const char *playlist_file = word_eol[3];
        
        /* Trim leading/trailing whitespace and quotes */
        while (*playlist_file == ' ' || *playlist_file == '\t') playlist_file++;
        
        char *filepath_copy = strdup(playlist_file);
        char *end = filepath_copy + strlen(filepath_copy) - 1;
        while (end > filepath_copy && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }
        
        /* Remove surrounding quotes if present */
        char *clean_path = filepath_copy;
        if (*clean_path == '"' || *clean_path == '\'') {
            clean_path++;
            end = clean_path + strlen(clean_path) - 1;
            if (*end == '"' || *end == '\'') {
                *end = '\0';
            }
        }
        
        /* Unescape shell escape sequences */
        char *unescaped = unescape_path(clean_path);
        
        int count = audioplayer_load_playlist_file(player, unescaped);
        if (count > 0) {
            print_msg("Loaded %d tracks from playlist: %s", count, unescaped);
        } else if (count == 0) {
            print_msg("No valid tracks found in playlist: %s", unescaped);
        } else {
            print_msg("Failed to load playlist (file not found or invalid format): %s", unescaped);
        }
        
        free(unescaped);
        free(filepath_copy);
    }
    else if (strcasecmp(subcmd, "clear") == 0) {
        if (audioplayer_clear_playlist(player) == 0) {
            print_msg("Playlist cleared");
        } else {
            print_msg("Failed to clear playlist");
        }
    }
    else if (strcasecmp(subcmd, "play") == 0) {
        /* Play first track in playlist */
        PlaylistItem *first = audioplayer_get_playlist(player);
        if (first) {
            if (audioplayer_play(player, first->filepath) == 0) {
                print_msg("Playing playlist: %s", first->title);
            } else {
                print_msg("Failed to play playlist");
            }
        } else {
            print_msg("Playlist is empty");
        }
    }
    else if (strcasecmp(subcmd, "loop") == 0) {
        if (word[3] && *word[3]) {
            bool enable = (strcasecmp(word[3], "on") == 0 || strcasecmp(word[3], "1") == 0);
            audioplayer_set_loop(player, enable);
            print_msg("Loop mode: %s", enable ? "ON" : "OFF");
        } else {
            print_msg("Usage: /PLAYLIST LOOP <on|off>");
        }
    }
    else if (strcasecmp(subcmd, "shuffle") == 0) {
        if (word[3] && *word[3]) {
            bool enable = (strcasecmp(word[3], "on") == 0 || strcasecmp(word[3], "1") == 0);
            audioplayer_set_shuffle(player, enable);
            print_msg("Shuffle mode: %s", enable ? "ON" : "OFF");
        } else {
            print_msg("Usage: /PLAYLIST SHUFFLE <on|off>");
        }
    }
    else {
        print_msg("Unknown playlist command. Available: ADD, LOAD, CLEAR, PLAY, LOOP, SHUFFLE");
    }
    
    return PCHAT_EAT_ALL;
}

/* /NP or /NOWPLAYING command - show current track */
static int nowplaying_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word_eol;
    (void)userdata;
    
    PlayerState state = audioplayer_get_state(player);
    
    if (state == STATE_STOPPED) {
        print_msg("Not playing anything");
        return PCHAT_EAT_ALL;
    }
    
    PlaylistItem *track = audioplayer_get_current_track(player);
    if (!track) {
        print_msg("No track information available");
        return PCHAT_EAT_ALL;
    }
    
    /* Build formatted track info with metadata */
    char track_info[512];
    if (track->artist && track->title) {
        /* Show "Artist - Title" if both available */
        snprintf(track_info, sizeof(track_info), "%s - %s", track->artist, track->title);
        if (track->album) {
            /* Add album if available */
            char temp[1024];
            snprintf(temp, sizeof(temp), "%s [%s]", track_info, track->album);
            strncpy(track_info, temp, sizeof(track_info) - 1);
        }
    } else if (track->title) {
        /* Just title if no artist */
        snprintf(track_info, sizeof(track_info), "%s", track->title);
    } else {
        /* Fallback to filename */
        snprintf(track_info, sizeof(track_info), "%s", track->filepath);
    }
    
    const char *state_str = (state == STATE_PLAYING) ? "Playing" : "Paused";
    print_msg("%s: %s", state_str, track_info);
    
    /* Optionally print to channel if specified */
    if (word[2] && strcasecmp(word[2], "say") == 0) {
        ph->pchat_commandf(ph, "SAY ♪ Now Playing: %s", track_info);
    }
    
    return PCHAT_EAT_ALL;
}

/* /GUI command - show GUI window */
static int gui_cb(char *word[], char *word_eol[], void *userdata) {
    (void)word;
    (void)word_eol;
    (void)userdata;
    
    audioplayer_gui_init(ph, player);
    return PCHAT_EAT_ALL;
}

/* Plugin initialization */
int pchat_plugin_init(pchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
    (void)arg;
    
    ph = plugin_handle;
    
    *plugin_name = PNAME;
    *plugin_desc = PDESC;
    *plugin_version = PVERSION;
    
    /* Create audio player instance */
    player = audioplayer_create();
    if (!player) {
        ph->pchat_print(ph, "AudioPlayer: Failed to initialize audio player\n");
        return 0;
    }
    
    /* Register commands */
    ph->pchat_hook_command(ph, "PLAY", PCHAT_PRI_NORM, play_cb, 
                      "PLAY <filepath> - Play an audio file", NULL);
    ph->pchat_hook_command(ph, "PAUSE", PCHAT_PRI_NORM, pause_cb,
                      "PAUSE - Pause or resume playback", NULL);
    ph->pchat_hook_command(ph, "STOP", PCHAT_PRI_NORM, stop_cb,
                      "STOP - Stop playback", NULL);
    ph->pchat_hook_command(ph, "NEXT", PCHAT_PRI_NORM, next_cb,
                      "NEXT - Play next track in playlist", NULL);
    ph->pchat_hook_command(ph, "PREV", PCHAT_PRI_NORM, prev_cb,
                      "PREV - Play previous track in playlist", NULL);
    ph->pchat_hook_command(ph, "PLAYLIST", PCHAT_PRI_NORM, playlist_cb,
                      "PLAYLIST [ADD|LOAD|CLEAR|PLAY|LOOP|SHUFFLE] - Manage playlist", NULL);
    ph->pchat_hook_command(ph, "NP", PCHAT_PRI_NORM, nowplaying_cb,
                      "NP [say] - Show now playing (optionally say to channel)", NULL);
    ph->pchat_hook_command(ph, "NOWPLAYING", PCHAT_PRI_NORM, nowplaying_cb,
                      "NOWPLAYING [say] - Show now playing (optionally say to channel)", NULL);
    ph->pchat_hook_command(ph, "APLAYER", PCHAT_PRI_NORM, gui_cb,
                      "APLAYER - Show audio player GUI", NULL);
    
    /* Add to user menu automatically */
    ph->pchat_command(ph, "MENU -p5 ADD \"Audio Player\" \"APLAYER\" \"\"");
    
    ph->pchat_print(ph, "AudioPlayer plugin loaded successfully!\n");
    ph->pchat_print(ph, "Commands: /PLAY, /PAUSE, /STOP, /NEXT, /PREV, /PLAYLIST, /NP, /APLAYER\n");
    ph->pchat_print(ph, "GUI: Use /APLAYER or check Window > Audio Player menu\n");
    
    return 1;
}

/* Plugin cleanup */
int pchat_plugin_deinit(void)
{
    /* Remove from user menu */
    ph->pchat_command(ph, "MENU DEL \"Audio Player\"");
    
    audioplayer_gui_cleanup();
    
    if (player) {
        audioplayer_destroy(player);
        player = NULL;
    }
    
    ph->pchat_print(ph, "AudioPlayer plugin unloaded\n");
    return 1;
}
