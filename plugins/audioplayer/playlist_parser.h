/* PChat Audio Player Plugin - Playlist File Parser
 * Copyright (C) 2025
 *
 * Parser for .m3u, .m3u8, and .pls playlist files
 */

#ifndef PCHAT_PLAYLIST_PARSER_H
#define PCHAT_PLAYLIST_PARSER_H

#include "audioplayer.h"

/* Supported playlist formats */
typedef enum {
    PLAYLIST_FORMAT_M3U,
    PLAYLIST_FORMAT_M3U8,
    PLAYLIST_FORMAT_PLS,
    PLAYLIST_FORMAT_UNKNOWN
} PlaylistFormat;

/* Parse result structure */
typedef struct {
    char **files;          /* Array of file paths */
    int count;             /* Number of files */
    PlaylistFormat format; /* Detected format */
} PlaylistParseResult;

/* Function declarations */
PlaylistFormat detect_playlist_format(const char *filepath);
PlaylistParseResult* parse_playlist_file(const char *filepath);
void free_playlist_result(PlaylistParseResult *result);

/* Specific format parsers */
int parse_m3u(const char *filepath, PlaylistParseResult *result);
int parse_pls(const char *filepath, PlaylistParseResult *result);

#endif /* PCHAT_PLAYLIST_PARSER_H */
