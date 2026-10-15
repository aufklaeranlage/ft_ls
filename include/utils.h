#ifndef FT_LS_UTILS_H
# define FT_LS_UTILS_H

# include <sys/stat.h>

const char *perm_str(mode_t mode, int has_acl);

char *make_path(char *dir, char *file);

#endif /* !FT_LS_UTILS_H */
