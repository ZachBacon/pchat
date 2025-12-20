/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

/* You can distribute this header with your plugins for easy compilation */
#ifndef PCHAT_PLUGIN_H
#define PCHAT_PLUGIN_H

#include <time.h>

#define PCHAT_PRI_HIGHEST	127
#define PCHAT_PRI_HIGH		64
#define PCHAT_PRI_NORM		0
#define PCHAT_PRI_LOW		(-64)
#define PCHAT_PRI_LOWEST	(-128)

#define PCHAT_FD_READ		1
#define PCHAT_FD_WRITE		2
#define PCHAT_FD_EXCEPTION	4
#define PCHAT_FD_NOTSOCKET	8

#define PCHAT_EAT_NONE		0	/* pass it on through! */
#define PCHAT_EAT_PCHAT		1	/* don't let PChat see this event */
#define PCHAT_EAT_PLUGIN	2	/* don't let other plugins see this event */
#define PCHAT_EAT_ALL		(PCHAT_EAT_PCHAT|PCHAT_EAT_PLUGIN)	/* don't let anything see this event */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _pchat_plugin pchat_plugin;
typedef struct _pchat_list pchat_list;
typedef struct _pchat_hook pchat_hook;
#ifndef PLUGIN_C
struct session;
typedef struct session pchat_context;
#endif
typedef struct
{
	time_t server_time_utc; /* 0 if not used */
} pchat_event_attrs;

#ifndef PLUGIN_C
struct _pchat_plugin
{
	/* these are only used on win32 */
	pchat_hook *(*pchat_hook_command) (pchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);
	pchat_hook *(*pchat_hook_server) (pchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);
	pchat_hook *(*pchat_hook_print) (pchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);
	pchat_hook *(*pchat_hook_timer) (pchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);
	pchat_hook *(*pchat_hook_fd) (pchat_plugin *ph,
		   int fd,
		   int flags,
		   int (*callback) (int fd, int flags, void *user_data),
		   void *userdata);
	void *(*pchat_unhook) (pchat_plugin *ph,
	      pchat_hook *hook);
	void (*pchat_print) (pchat_plugin *ph,
	     const char *text);
	void (*pchat_printf) (pchat_plugin *ph,
	      const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
	;
	void (*pchat_command) (pchat_plugin *ph,
	       const char *command);
	void (*pchat_commandf) (pchat_plugin *ph,
		const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
	;
	int (*pchat_nickcmp) (pchat_plugin *ph,
	       const char *s1,
	       const char *s2);
	int (*pchat_set_context) (pchat_plugin *ph,
		   pchat_context *ctx);
	pchat_context *(*pchat_find_context) (pchat_plugin *ph,
		    const char *servname,
		    const char *channel);
	pchat_context *(*pchat_get_context) (pchat_plugin *ph);
	const char *(*pchat_get_info) (pchat_plugin *ph,
		const char *id);
	int (*pchat_get_prefs) (pchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);
	pchat_list * (*pchat_list_get) (pchat_plugin *ph,
		const char *name);
	void (*pchat_list_free) (pchat_plugin *ph,
		 pchat_list *xlist);
	const char * const * (*pchat_list_fields) (pchat_plugin *ph,
		   const char *name);
	int (*pchat_list_next) (pchat_plugin *ph,
		 pchat_list *xlist);
	const char * (*pchat_list_str) (pchat_plugin *ph,
		pchat_list *xlist,
		const char *name);
	int (*pchat_list_int) (pchat_plugin *ph,
		pchat_list *xlist,
		const char *name);
	void * (*pchat_plugingui_add) (pchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);
	void (*pchat_plugingui_remove) (pchat_plugin *ph,
			void *handle);
	int (*pchat_emit_print) (pchat_plugin *ph,
			const char *event_name, ...);
	int (*pchat_read_fd) (pchat_plugin *ph,
			void *src,
			char *buf,
			int *len);
	time_t (*pchat_list_time) (pchat_plugin *ph,
		pchat_list *xlist,
		const char *name);
	char *(*pchat_gettext) (pchat_plugin *ph,
		const char *msgid);
	void (*pchat_send_modes) (pchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);
	char *(*pchat_strip) (pchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);
	void (*pchat_free) (pchat_plugin *ph,
	    void *ptr);
	int (*pchat_pluginpref_set_str) (pchat_plugin *ph,
		const char *var,
		const char *value);
	int (*pchat_pluginpref_get_str) (pchat_plugin *ph,
		const char *var,
		char *dest);
	int (*pchat_pluginpref_set_int) (pchat_plugin *ph,
		const char *var,
		int value);
	int (*pchat_pluginpref_get_int) (pchat_plugin *ph,
		const char *var);
	int (*pchat_pluginpref_delete) (pchat_plugin *ph,
		const char *var);
	int (*pchat_pluginpref_list) (pchat_plugin *ph,
		char *dest);
	pchat_hook *(*pchat_hook_server_attrs) (pchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[],
							pchat_event_attrs *attrs, void *user_data),
		   void *userdata);
	pchat_hook *(*pchat_hook_print_attrs) (pchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], pchat_event_attrs *attrs,
						   void *user_data),
		  void *userdata);
	int (*pchat_emit_print_attrs) (pchat_plugin *ph, pchat_event_attrs *attrs,
									 const char *event_name, ...);
	pchat_event_attrs *(*pchat_event_attrs_create) (pchat_plugin *ph);
	void (*pchat_event_attrs_free) (pchat_plugin *ph,
									  pchat_event_attrs *attrs);
};
#endif


pchat_hook *
pchat_hook_command (pchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);

pchat_event_attrs *pchat_event_attrs_create (pchat_plugin *ph);

void pchat_event_attrs_free (pchat_plugin *ph, pchat_event_attrs *attrs);

pchat_hook *
pchat_hook_server (pchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);

pchat_hook *
pchat_hook_server_attrs (pchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[],
							pchat_event_attrs *attrs, void *user_data),
		   void *userdata);

pchat_hook *
pchat_hook_print (pchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);

pchat_hook *
pchat_hook_print_attrs (pchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], pchat_event_attrs *attrs,
						   void *user_data),
		  void *userdata);

pchat_hook *
pchat_hook_timer (pchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);

pchat_hook *
pchat_hook_fd (pchat_plugin *ph,
		int fd,
		int flags,
		int (*callback) (int fd, int flags, void *user_data),
		void *userdata);

void *
pchat_unhook (pchat_plugin *ph,
	      pchat_hook *hook);

void
pchat_print (pchat_plugin *ph,
	     const char *text);

void
pchat_printf (pchat_plugin *ph,
	      const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
;

void
pchat_command (pchat_plugin *ph,
	       const char *command);

void
pchat_commandf (pchat_plugin *ph,
		const char *format, ...)
#ifdef __GNUC__
	__attribute__((format(printf, 2, 3)))
#endif
;

int
pchat_nickcmp (pchat_plugin *ph,
	       const char *s1,
	       const char *s2);

int
pchat_set_context (pchat_plugin *ph,
		   pchat_context *ctx);

pchat_context *
pchat_find_context (pchat_plugin *ph,
		    const char *servname,
		    const char *channel);

pchat_context *
pchat_get_context (pchat_plugin *ph);

const char *
pchat_get_info (pchat_plugin *ph,
		const char *id);

int
pchat_get_prefs (pchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);

pchat_list *
pchat_list_get (pchat_plugin *ph,
		const char *name);

void
pchat_list_free (pchat_plugin *ph,
		 pchat_list *xlist);

const char * const *
pchat_list_fields (pchat_plugin *ph,
		   const char *name);

int
pchat_list_next (pchat_plugin *ph,
		 pchat_list *xlist);

const char *
pchat_list_str (pchat_plugin *ph,
		pchat_list *xlist,
		const char *name);

int
pchat_list_int (pchat_plugin *ph,
		pchat_list *xlist,
		const char *name);

time_t
pchat_list_time (pchat_plugin *ph,
		 pchat_list *xlist,
		 const char *name);

void *
pchat_plugingui_add (pchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);

void
pchat_plugingui_remove (pchat_plugin *ph,
			void *handle);

int 
pchat_emit_print (pchat_plugin *ph,
		  const char *event_name, ...);

int 
pchat_emit_print_attrs (pchat_plugin *ph, pchat_event_attrs *attrs,
						  const char *event_name, ...);

char *
pchat_gettext (pchat_plugin *ph,
	       const char *msgid);

void
pchat_send_modes (pchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);

char *
pchat_strip (pchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);

void
pchat_free (pchat_plugin *ph,
	    void *ptr);

int
pchat_pluginpref_set_str (pchat_plugin *ph,
		const char *var,
		const char *value);

int
pchat_pluginpref_get_str (pchat_plugin *ph,
		const char *var,
		char *dest);

int
pchat_pluginpref_set_int (pchat_plugin *ph,
		const char *var,
		int value);
int
pchat_pluginpref_get_int (pchat_plugin *ph,
		const char *var);

int
pchat_pluginpref_delete (pchat_plugin *ph,
		const char *var);

int
pchat_pluginpref_list (pchat_plugin *ph,
		char *dest);

#if !defined(PLUGIN_C) && (defined(WIN32) || defined(__CYGWIN__))
#ifndef pchat_PLUGIN_HANDLE
#define pchat_PLUGIN_HANDLE (ph)
#endif
#define pchat_hook_command ((pchat_PLUGIN_HANDLE)->pchat_hook_command)
#define pchat_event_attrs_create ((pchat_PLUGIN_HANDLE)->pchat_event_attrs_create)
#define pchat_event_attrs_free ((pchat_PLUGIN_HANDLE)->pchat_event_attrs_free)
#define pchat_hook_server ((pchat_PLUGIN_HANDLE)->pchat_hook_server)
#define pchat_hook_server_attrs ((pchat_PLUGIN_HANDLE)->pchat_hook_server_attrs)
#define pchat_hook_print ((pchat_PLUGIN_HANDLE)->pchat_hook_print)
#define pchat_hook_print_attrs ((pchat_PLUGIN_HANDLE)->pchat_hook_print_attrs)
#define pchat_hook_timer ((pchat_PLUGIN_HANDLE)->pchat_hook_timer)
#define pchat_hook_fd ((pchat_PLUGIN_HANDLE)->pchat_hook_fd)
#define pchat_unhook ((pchat_PLUGIN_HANDLE)->pchat_unhook)
#define pchat_print ((pchat_PLUGIN_HANDLE)->pchat_print)
#define pchat_printf ((pchat_PLUGIN_HANDLE)->pchat_printf)
#define pchat_command ((pchat_PLUGIN_HANDLE)->pchat_command)
#define pchat_commandf ((pchat_PLUGIN_HANDLE)->pchat_commandf)
#define pchat_nickcmp ((pchat_PLUGIN_HANDLE)->pchat_nickcmp)
#define pchat_set_context ((pchat_PLUGIN_HANDLE)->pchat_set_context)
#define pchat_find_context ((pchat_PLUGIN_HANDLE)->pchat_find_context)
#define pchat_get_context ((pchat_PLUGIN_HANDLE)->pchat_get_context)
#define pchat_get_info ((pchat_PLUGIN_HANDLE)->pchat_get_info)
#define pchat_get_prefs ((pchat_PLUGIN_HANDLE)->pchat_get_prefs)
#define pchat_list_get ((pchat_PLUGIN_HANDLE)->pchat_list_get)
#define pchat_list_free ((pchat_PLUGIN_HANDLE)->pchat_list_free)
#define pchat_list_fields ((pchat_PLUGIN_HANDLE)->pchat_list_fields)
#define pchat_list_next ((pchat_PLUGIN_HANDLE)->pchat_list_next)
#define pchat_list_str ((pchat_PLUGIN_HANDLE)->pchat_list_str)
#define pchat_list_int ((pchat_PLUGIN_HANDLE)->pchat_list_int)
#define pchat_plugingui_add ((pchat_PLUGIN_HANDLE)->pchat_plugingui_add)
#define pchat_plugingui_remove ((pchat_PLUGIN_HANDLE)->pchat_plugingui_remove)
#define pchat_emit_print ((pchat_PLUGIN_HANDLE)->pchat_emit_print)
#define pchat_emit_print_attrs ((pchat_PLUGIN_HANDLE)->pchat_emit_print_attrs)
#define pchat_list_time ((pchat_PLUGIN_HANDLE)->pchat_list_time)
#define pchat_gettext ((pchat_PLUGIN_HANDLE)->pchat_gettext)
#define pchat_send_modes ((pchat_PLUGIN_HANDLE)->pchat_send_modes)
#define pchat_strip ((pchat_PLUGIN_HANDLE)->pchat_strip)
#define pchat_free ((pchat_PLUGIN_HANDLE)->pchat_free)
#define pchat_pluginpref_set_str ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_set_str)
#define pchat_pluginpref_get_str ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_get_str)
#define pchat_pluginpref_set_int ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_set_int)
#define pchat_pluginpref_get_int ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_get_int)
#define pchat_pluginpref_delete ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_delete)
#define pchat_pluginpref_list ((pchat_PLUGIN_HANDLE)->pchat_pluginpref_list)
#endif

#ifdef __cplusplus
}
#endif
#endif
