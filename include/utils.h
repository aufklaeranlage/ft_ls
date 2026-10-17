#ifndef FT_LS_UTILS_H
# define FT_LS_UTILS_H

# include <sys/stat.h>
# include <sys/types.h>
# include <fts.h>

extern const char *NS_PERM_STR;

const char *perm_str(mode_t mode);

char *make_path(char *dir, char *file);

int fts_alphasort(const FTSENT **f1, const FTSENT **f2);

int fts_mtimesort(const FTSENT **f1, const FTSENT **f2);

int fts_atimesort(const FTSENT **f1, const FTSENT **f2);

#endif /* !FT_LS_UTILS_H */
