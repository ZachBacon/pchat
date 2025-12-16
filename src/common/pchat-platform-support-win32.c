#include "pchat-platform-support.h"

#include <glib.h>
//#include <gio/gwin32inputstream.h>

#define SAVE_DATADIR DATADIR
#undef DATADIR
#include <io.h>
#include <conio.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#define DATADIR SAVE_DATADIR
#undef SAVE_DATADIR



char *
pchat_platform_support_get_lib_dir (void)
{
	char *module_dir;
	char *lib_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	lib_dir = g_build_filename (module_dir, "lib", "pchat", NULL);
	g_free (module_dir);

	return lib_dir;
}

char *
pchat_platform_support_get_locale_dir (void)
{
	char *module_dir;
	char *locale_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	locale_dir = g_build_filename (module_dir, "share", "locale", NULL);
	g_free (module_dir);

	return locale_dir;
}

char *
pchat_platform_support_get_data_dir (void)
{
	char *module_dir;
	char *data_dir;

	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	data_dir = g_build_filename (module_dir, "share", "pchat", NULL);
	g_free (module_dir);

	return data_dir;
}

char *
pchat_platform_support_get_user_home_dir (const char *name)
{
	// TODO
	return NULL;
}

