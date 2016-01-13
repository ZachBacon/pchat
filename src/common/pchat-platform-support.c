/*
 * This file is inspired by gitg's platform support code and will allow a more fhs install on a variety of platforms
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pchat-platform-support.h"

gchar *
pchat_platform_support_get_lib_dir (void)
{
	return g_strdup (XCHATLIBDIR);
}

gchar *
pchat_platform_support_get_locale_dir (void)
{
	return g_strdup (ISO_CODES_LOCALEDIR);
}

gchar *
pchat_platform_support_get_data_dir (void)
{
	return g_strdup (PCHAT_DATADIR);
}

gchar *
pchat_platform_support_get_user_home_dir (const gchar *name)
{
	struct passwd *pwd;

	if (name == NULL)
	{
		name = g_get_user_name ();
	}

	if (name == NULL)
	{
		return NULL;
	}

	pwd = getpwnam (name);

	if (pwd == NULL)
	{
		return NULL;
	}

	return g_strdup (pwd->pw_dir);
}
