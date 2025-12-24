/* PChat Debug Logging Header
 * Copyright (C) 2025 Zach Bacon
 */

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Only enable debug logging in debug builds */
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define PCHAT_DEBUG_LOGGING 1
#endif

#ifdef PCHAT_DEBUG_LOGGING

void debug_log_init(const char *log_path);
void debug_log_write(const char *component, const char *format, ...);
void debug_log_close(void);

#define DEBUG_LOG(component, ...) debug_log_write(component, __VA_ARGS__)

#else

/* No-op macros for release builds */
#define debug_log_init(log_path) ((void)0)
#define debug_log_write(component, format, ...) ((void)0)
#define debug_log_close() ((void)0)
#define DEBUG_LOG(component, ...) ((void)0)

#endif /* PCHAT_DEBUG_LOGGING */

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_LOG_H */
