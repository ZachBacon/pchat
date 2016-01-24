/*
 * This file is inspired by gitg's platform support code and will allow a more fhs install on a variety of platforms
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pchat-platform-support.h"

char *
pchat_platform_support_get_lib_dir (void)
{
	return g_strdup (XCHATLIBDIR);
}

char *
pchat_platform_support_get_locale_dir (void)
{
	return g_strdup (ISO_CODES_LOCALEDIR);
}

char *
pchat_platform_support_get_data_dir (void)
{
	return g_strdup (ISO_CODES_LOCALEDIR);
}

char *
pchat_platform_support_get_user_home_dir (const char *name)
{
	return NULL;
}
