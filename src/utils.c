#include <sys/stat.h>
#include <sys/types.h>
#include <fts.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

const char *NS_PERM_STR = "-?????????";

const char *perm_str(mode_t mode)
{
	static char str[11];
	str[10] = 0;
	switch (S_IFMT & mode) {
		case S_IFSOCK: str[0] = 's'; break ;
		case S_IFLNK: str[0] = 'l'; break ;
		case S_IFBLK: str[0] = 'b'; break ;
		case S_IFDIR: str[0] = 'd'; break ;
		case S_IFCHR: str[0] = 'c'; break ;
		case S_IFIFO: str[0] = 'p'; break ;
		default: str[0] = '-';
	}
	str[1] = mode & S_IRUSR ? 'r' : '-';
	str[2] = mode & S_IWUSR ? 'w' : '-';
	if (mode & S_ISUID)
		str[3] = mode & S_IXUSR ? 's' : 'S';
	else
		str[3] = mode & S_IXUSR ? 'x' : '-';
	str[4] = mode & S_IRGRP ? 'r' : '-';
	str[5] = mode & S_IWGRP ? 'w' : '-';
	if (mode & S_ISGID)
		str[6] = mode & S_IXGRP ? 's' : 'S';
	else
		str[6] = mode & S_IXGRP ? 'x' : '-';
	str[7] = mode & S_IROTH ? 'r' : '-';
	str[8] = mode & S_IWOTH ? 'w' : '-';
	if (mode & S_ISVTX)
		str[9] = mode & S_IXOTH ? 't' : 'T';
	else
		str[9] = mode & S_IXOTH ? 'x' : '-';
	return str;
}

/*!
 * @brief Creates a concatinated syntactically valid path out of `dir` and `file`.
 * @parameter `dir` the path to the directory that `file` should be apended to.
 * @parameter `file` the file name that should be appended to `dir`.
 * @return Returns a concatinated syntactically valid path on success; NULL on
 *	failure.
 */
char *make_path(char *dir, char *file)
{
	char *path;
	int dirlen = strlen(dir), filelen = strlen(file);
	int needs_delim = dir[dirlen - 1] == '/' ? 0 : 1;
	int total_len = dirlen + filelen + needs_delim;
	path = malloc(sizeof(*path) * (total_len + 1));
	if (!path)
		return NULL;
	strcpy(path, dir);
	if (needs_delim)
		path[dirlen] = '/';
	strcpy(path + dirlen + needs_delim, file);
	return path;
}

static __attribute__((unused)) int cistrcmp(const char *s1, const char *s2)
{
	int ret = 0;
	int pos1 = 0, pos2 = 0;
	while (s1[pos1] == '.')
		++pos1;
	while (s2[pos2] == '.')
		++pos2;
	while (s1[pos1] || s2[pos2]) {
		ret = toupper(s1[pos1++]);
		ret -= toupper(s2[pos2++]);
		if (ret)
			return ret;
	}
	return pos1 - pos2;
}

static int ftsent_has_err(const FTSENT *ent)
{
	return (ent->fts_info == FTS_ERR
			|| ent->fts_info == FTS_DNR
			|| ent->fts_info == FTS_NS
			|| ent->fts_info == FTS_NSOK);
}

int fts_alphasort(const FTSENT **f1, const FTSENT **f2)
{
	int f1err = ftsent_has_err(*f1),
			f2err = ftsent_has_err(*f2);
	switch (f1err + f2err) {
		case 1:
			return f2err - f1err;
		case 2:
			return 1;
	}
	return strcoll((*f1)->fts_name, (*f2)->fts_name);
}

int fts_mtimesort(const FTSENT **f1, const FTSENT **f2)
{
	if (ftsent_has_err(*f1) || ftsent_has_err(*f2))
		return fts_alphasort(f1, f2);
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
	int diff_sec = (*f2)->fts_statp->st_mtime - (*f1)->fts_statp->st_mtime;
	if (!diff_sec)
		return (*f2)->fts_statp->st_mtim.tv_nsec - (*f1)->fts_statp->st_mtim.tv_nsec;
	return diff_sec;
#else
	return (*f2)->fts_statp->st_mtime - (*f1)->fts_statp->st_mtime;
#endif
}

int fts_atimesort(const FTSENT **f1, const FTSENT **f2)
{
	if (ftsent_has_err(*f1) || ftsent_has_err(*f2))
		return fts_alphasort(f1, f2);
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
	int diff_sec = (*f2)->fts_statp->st_atime - (*f1)->fts_statp->st_atime;
	if (!diff_sec)
		return (*f2)->fts_statp->st_atim.tv_nsec - (*f1)->fts_statp->st_atim.tv_nsec;
	return diff_sec;
#else
	return (*f2)->fts_statp->st_atime - (*f1)->fts_statp->st_atime;
#endif
}

int fts_ralphasort(const FTSENT **f1, const FTSENT **f2)
{
	int f1err = ftsent_has_err(*f1),
			f2err = ftsent_has_err(*f2);
	switch (f1err + f2err) {
		case 1:
			return f2err - f1err;
		case 2:
			return 1;
	}
	return cistrcmp((*f2)->fts_name, (*f1)->fts_name);
}

int fts_rmtimesort(const FTSENT **f1, const FTSENT **f2)
{
	if (ftsent_has_err(*f1) || ftsent_has_err(*f2))
		return fts_ralphasort(f1, f2);
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
	int diff_sec = (*f1)->fts_statp->st_mtime - (*f2)->fts_statp->st_mtime;
	if (!diff_sec)
		return (*f1)->fts_statp->st_mtim.tv_nsec - (*f2)->fts_statp->st_mtim.tv_nsec;
	return diff_sec;
#else
	return (*f1)->fts_statp->st_mtime - (*f2)->fts_statp->st_mtime;
#endif
}

int fts_ratimesort(const FTSENT **f1, const FTSENT **f2)
{
	if (ftsent_has_err(*f1) || ftsent_has_err(*f2))
		return fts_ralphasort(f1, f2);
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
	int diff_sec = (*f1)->fts_statp->st_atime - (*f2)->fts_statp->st_atime;
	if (!diff_sec)
		return (*f1)->fts_statp->st_atim.tv_nsec - (*f2)->fts_statp->st_atim.tv_nsec;
	return diff_sec;
#else
	return (*f1)->fts_statp->st_atime - (*f2)->fts_statp->st_atime;
#endif
}
