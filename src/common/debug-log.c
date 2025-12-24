/* PChat Debug Logging
 * Copyright (C) 2025 Zach Bacon
 * Simple file-based debug logging for troubleshooting
 */

#include "debug-log.h"

/* Only compile debug logging code in debug builds */
#ifdef PCHAT_DEBUG_LOGGING

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP "\\"
#else
#include <sys/stat.h>
#include <sys/types.h>
#define PATH_SEP "/"
#endif

static FILE *debug_log_file = NULL;
static int debug_log_enabled = 0;

void debug_log_init(const char *log_path)
{
	debug_log_enabled = 1;
	
	if (!log_path)
	{
		log_path = "pchat-debug.log";
	}
	
	debug_log_file = fopen(log_path, "w");
	if (debug_log_file)
	{
		fprintf(debug_log_file, "=== PChat Debug Log Started ===\n");
		fflush(debug_log_file);
	}
}

void debug_log_write(const char *component, const char *format, ...)
{
	if (!debug_log_enabled || !debug_log_file)
		return;
	
	time_t now;
	struct tm *timeinfo;
	char time_buf[32];
	va_list args;
	
	time(&now);
	timeinfo = localtime(&now);
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
	
	fprintf(debug_log_file, "[%s] [%s] ", time_buf, component);
	
	va_start(args, format);
	vfprintf(debug_log_file, format, args);
	va_end(args);
	
	fprintf(debug_log_file, "\n");
	fflush(debug_log_file);
}

void debug_log_close(void)
{
	if (debug_log_file)
	{
		fprintf(debug_log_file, "=== PChat Debug Log Ended ===\n");
		fflush(debug_log_file);
		fclose(debug_log_file);
		debug_log_file = NULL;
	}
	debug_log_enabled = 0;
}

#endif /* PCHAT_DEBUG_LOGGING */
