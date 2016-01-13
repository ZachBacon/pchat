#ifndef __PCHAT_PLATFORM_SUPPORT_H__
#define __PCHAT_PLATFORM_SUPPORT_H__

char        *pchat_platform_support_get_lib_dir (void);
char        *pchat_platform_support_get_locale_dir (void);
char        *pchat_platform_support_get_data_dir (void);
char        *pchat_platform_support_get_user_home_dir (const char *name);

#endif /* __PCHAT_PLATFORM_SUPPORT_H__ */
