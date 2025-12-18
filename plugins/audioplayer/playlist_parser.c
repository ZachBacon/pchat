/* PChat Audio Player Plugin - Playlist File Parser
 * Copyright (C) 2025
 *
 * Implementation of .m3u, .m3u8, and .pls playlist parsers
 */

#include "playlist_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>

/* Helper function to check if file exists */
static int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/* Helper function to resolve relative paths */
static char* resolve_path(const char *playlist_dir, const char *filename) {
    /* If absolute path, return as-is */
    if (filename[0] == '/' || filename[0] == '~') {
        return strdup(filename);
    }
    
    /* If relative, combine with playlist directory */
    size_t dir_len = strlen(playlist_dir);
    size_t file_len = strlen(filename);
    char *resolved = malloc(dir_len + file_len + 2); /* +2 for / and \0 */
    
    if (!resolved) {
        return NULL;
    }
    
    snprintf(resolved, dir_len + file_len + 2, "%s/%s", playlist_dir, filename);
    return resolved;
}

/* Helper function to trim whitespace */
static char* trim_whitespace(char *str) {
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) {
        return str;
    }
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

/* Detect playlist format from file extension */
PlaylistFormat detect_playlist_format(const char *filepath) {
    const char *ext = strrchr(filepath, '.');
    
    if (!ext) {
        return PLAYLIST_FORMAT_UNKNOWN;
    }
    
    ext++; /* Skip the dot */
    
    if (strcasecmp(ext, "m3u") == 0) {
        return PLAYLIST_FORMAT_M3U;
    } else if (strcasecmp(ext, "m3u8") == 0) {
        return PLAYLIST_FORMAT_M3U8;
    } else if (strcasecmp(ext, "pls") == 0) {
        return PLAYLIST_FORMAT_PLS;
    }
    
    return PLAYLIST_FORMAT_UNKNOWN;
}

/* Parse M3U/M3U8 playlist file */
int parse_m3u(const char *filepath, PlaylistParseResult *result) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("fopen");
        fprintf(stderr, "Failed to open playlist file: %s\n", filepath);
        return -1;
    }
    
    /* Get playlist directory for resolving relative paths */
    char *filepath_copy = strdup(filepath);
    char *playlist_dir = dirname(filepath_copy);
    
    /* First pass: count valid entries */
    char line[4096];
    int count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        
        /* Check if file exists */
        char *resolved = resolve_path(playlist_dir, trimmed);
        if (resolved) {
            if (file_exists(resolved)) {
                count++;
            } else {
                fprintf(stderr, "File not found: %s (resolved from: %s)\n", resolved, trimmed);
            }
            free(resolved);
        }
    }
    
    if (count == 0) {
        fprintf(stderr, "No valid files found in M3U playlist: %s\n", filepath);
        fclose(fp);
        free(filepath_copy);
        return -1;
    }
    
    /* Allocate array */
    result->files = malloc(sizeof(char*) * count);
    if (!result->files) {
        fclose(fp);
        free(filepath_copy);
        return -1;
    }
    
    /* Second pass: collect files */
    rewind(fp);
    int index = 0;
    
    while (fgets(line, sizeof(line), fp) && index < count) {
        char *trimmed = trim_whitespace(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        
        /* Resolve and add if exists */
        char *resolved = resolve_path(playlist_dir, trimmed);
        if (resolved && file_exists(resolved)) {
            result->files[index++] = resolved;
        } else {
            free(resolved);
        }
    }
    
    result->count = index;
    result->format = PLAYLIST_FORMAT_M3U;
    
    fclose(fp);
    free(filepath_copy);
    return 0;
}

/* Parse PLS playlist file */
int parse_pls(const char *filepath, PlaylistParseResult *result) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return -1;
    }
    
    /* Get playlist directory for resolving relative paths */
    char *filepath_copy = strdup(filepath);
    char *playlist_dir = dirname(filepath_copy);
    
    char line[4096];
    int in_playlist_section = 0;
    int number_of_entries = 0;
    
    /* First pass: get number of entries */
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        
        if (strcasecmp(trimmed, "[playlist]") == 0) {
            in_playlist_section = 1;
            continue;
        }
        
        if (in_playlist_section && strncasecmp(trimmed, "NumberOfEntries=", 16) == 0) {
            number_of_entries = atoi(trimmed + 16);
            break;
        }
    }
    
    if (number_of_entries == 0) {
        /* No explicit count, we'll count File entries */
        rewind(fp);
        in_playlist_section = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            char *trimmed = trim_whitespace(line);
            
            if (strcasecmp(trimmed, "[playlist]") == 0) {
                in_playlist_section = 1;
                continue;
            }
            
            if (in_playlist_section && strncasecmp(trimmed, "File", 4) == 0) {
                number_of_entries++;
            }
        }
    }
    
    if (number_of_entries == 0) {
        fclose(fp);
        free(filepath_copy);
        return -1;
    }
    
    /* Allocate array */
    result->files = malloc(sizeof(char*) * number_of_entries);
    if (!result->files) {
        fclose(fp);
        free(filepath_copy);
        return -1;
    }
    
    /* Second pass: collect File entries */
    rewind(fp);
    in_playlist_section = 0;
    int index = 0;
    
    while (fgets(line, sizeof(line), fp) && index < number_of_entries) {
        char *trimmed = trim_whitespace(line);
        
        if (strcasecmp(trimmed, "[playlist]") == 0) {
            in_playlist_section = 1;
            continue;
        }
        
        if (in_playlist_section && strncasecmp(trimmed, "File", 4) == 0) {
            /* Find the = sign */
            char *equals = strchr(trimmed, '=');
            if (equals) {
                char *filename = trim_whitespace(equals + 1);
                
                /* Resolve and add if exists */
                char *resolved = resolve_path(playlist_dir, filename);
                if (resolved && file_exists(resolved)) {
                    result->files[index++] = resolved;
                } else {
                    free(resolved);
                }
            }
        }
    }
    
    result->count = index;
    result->format = PLAYLIST_FORMAT_PLS;
    
    fclose(fp);
    free(filepath_copy);
    return 0;
}

/* Parse playlist file (auto-detect format) */
PlaylistParseResult* parse_playlist_file(const char *filepath) {
    PlaylistFormat format = detect_playlist_format(filepath);
    
    if (format == PLAYLIST_FORMAT_UNKNOWN) {
        return NULL;
    }
    
    PlaylistParseResult *result = malloc(sizeof(PlaylistParseResult));
    if (!result) {
        return NULL;
    }
    
    memset(result, 0, sizeof(PlaylistParseResult));
    
    int parse_result = -1;
    
    switch (format) {
        case PLAYLIST_FORMAT_M3U:
        case PLAYLIST_FORMAT_M3U8:
            parse_result = parse_m3u(filepath, result);
            break;
            
        case PLAYLIST_FORMAT_PLS:
            parse_result = parse_pls(filepath, result);
            break;
            
        default:
            free(result);
            return NULL;
    }
    
    if (parse_result < 0) {
        free(result);
        return NULL;
    }
    
    return result;
}

/* Free playlist parse result */
void free_playlist_result(PlaylistParseResult *result) {
    if (!result) {
        return;
    }
    
    for (int i = 0; i < result->count; i++) {
        free(result->files[i]);
    }
    
    free(result->files);
    free(result);
}
