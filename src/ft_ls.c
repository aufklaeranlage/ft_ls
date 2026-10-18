#include "ft_ls.h"
#include "utils.h"

#include <sys/types.h>
#include <sys/xattr.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <langinfo.h>
#include <stdint.h>

const char *prog = "ft_ls";

// static void print_usage(void)
// {
//
// }

static int read_args(struct args *args, int ac, char *av[])
{
	*args = (struct args){ };
	int opt;
	while ((opt = getopt(ac, av, "lgRartud")) != -1) {
		switch (opt) {
		case 'l': args->long_fmt = 1; break;
		case 'g': args->long_no_owner = 1; break;
		case 'R': args->recursive = 1; break;
		case 'a': args->all = 1; break;
		case 'r': args->reverse = 1; break;
		case 't': args->sort_time = 1; break;
		case 'u': args->show_access_time = 1; break;
		case 'd': args->list_directories = 1; break;
		default:
							return -1;
		}
	}
	return optind;
}

extern const char *NS_PERM_STR;

struct print_fmt_len {
	int acl;
	int nlink;
	int user;
	int group;
	int bsize;
	int date;
};

static int ndigits(int num, unsigned int base)
{
	int size = 1;
	while (num > 9) {
		num /= base;
		++size;
	}
	return size;
}

int has_acl(const char *pfad) {
    const char *acl_attr = "system.posix_acl_access";
    ssize_t size = lgetxattr(pfad, acl_attr, NULL, 0);
    if (size < 0) {
        if (errno == ENODATA || errno == EOPNOTSUPP || errno == ENOTSUP)
            return 0; 
        return -1;
    }
    if (size > 24)
        return 1;
    return 0;
}

static int get_print_fmt_len(struct print_fmt_len *pfl, FTSENT *head, struct args *args)
{
	*pfl = (struct print_fmt_len) { };
	int digits;
	struct passwd *pwd;
	struct group *grp;
	struct tm *tm;
	char datestring[256];
	const char *name;
	for (FTSENT *node = head; node != NULL; node = node->fts_link) {
		int len;
		name = node->fts_name; 
		if (!args->all && name[0] == '.')
			continue ;
		if (has_acl(node->fts_path))
			pfl->acl = 1;
		switch (node->fts_info) {
				case FTS_ERR:
				case FTS_NS:
				case FTS_NSOK:
					continue;
				default:
					break ;
		}
		digits = ndigits(node->fts_statp->st_nlink, 10);
		if (digits > pfl->nlink)
			pfl->nlink = digits;
		if ((pwd = getpwuid(node->fts_statp->st_uid)) != NULL
				&& (len = strlen(pwd->pw_name)) > pfl->user)
			pfl->user = len;
		if ((grp = getgrgid(node->fts_statp->st_gid)) != NULL
				&& (len = strlen(grp->gr_name)) > pfl->group)
			pfl->group = len;
		digits = ndigits(node->fts_statp->st_size, 10);
		if (digits > pfl->bsize)
			pfl->bsize = digits;
		tm = args->show_access_time ?
			localtime(&node->fts_statp->st_atime) :
			localtime(&node->fts_statp->st_mtime);
		if ((len = strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm)) > pfl->date)
			pfl->date = len;
	}
	return 1;
}

static int fts_print_dir(FTSENT *head, struct args *args, int print_name)
{
	if (!head)
		return 0;
	if (print_name)
		fprintf(stdout, "%s:\n", head->fts_parent->fts_accpath);
	struct print_fmt_len pfl;
	if (args->long_fmt || args->long_no_owner)
		get_print_fmt_len(&pfl, head, args);
	const char *name;
	const char *permstr;
	int first = 1;
	for (FTSENT *node = head; node != NULL; node = node->fts_link) {
		name = node->fts_name; 
		if (!args->all && name[0] == '.')
			continue ;
		if (!args->long_fmt && !args->long_no_owner) {
			fprintf(stdout, "%s%s", first ? "" : "  ", name);
		} else {
			switch (node->fts_info) {
				case FTS_DP:
					continue;
				case FTS_DOT:
					if (!args->all)
						continue;
				case FTS_D:
				case FTS_F:
					permstr = perm_str(node->fts_statp->st_mode); break;
				case FTS_ERR:
				case FTS_NS:
				case FTS_NSOK:
				default:
					permstr = NS_PERM_STR;
			}
			struct passwd *pwd;
			struct group *grp;
			char datestring[256];
			int dslen;
			struct tm *tm = args->show_access_time ?
				localtime(&node->fts_statp->st_atime) :
				localtime(&node->fts_statp->st_mtime);
			dslen = strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
			fprintf(stdout, "%s%s %*ju %*s %*s %*jd %*s %s\n",
					permstr,
					pfl.acl ? (has_acl(node->fts_path) ? "+" : " ") : "",
					pfl.nlink, (uintmax_t) node->fts_statp->st_nlink,
					pfl.user, ((pwd = getpwuid(node->fts_statp->st_uid)) != NULL) ? pwd->pw_name : "?", 
					pfl.group, ((grp = getgrgid(node->fts_statp->st_gid)) != NULL) ? grp->gr_name : "?", 
					pfl.bsize, (intmax_t) node->fts_statp->st_size,
					pfl.date, dslen > 0 ? datestring : "?",
					name);
		}
		first = 0;
	}
	return 1;
}

static int get_nprint(FTS *fts, struct args *args)
{
	if (!fts)
		return -1;
	int num = 0;
	for (FTSENT *ent = fts_read(fts); ent != NULL; ent = fts_read(fts)) {
		if (ent->fts_info == FTS_DP)
			continue ;
		const char *name = ent->fts_name;
		if (ent->fts_level > 0 && (!args->recursive
				|| (!args->all && name[0] == '.'))) {
			fts_set(fts, ent, FTS_SKIP);
			continue ;
		}
		if (ent->fts_info == FTS_D || ent->fts_level == 0)
			++num;
	}
	fts_close(fts);
	return num;
}

int main(int ac, char *av[])
{
	prog = av[0];
	struct args args;
	char *fallback_pathav[2] = { ".", NULL };

	int pos = read_args(&args, ac, av);
	if (pos < 0)
		return 1;
	char **pathav = pos >= ac ? fallback_pathav : av + pos;
	int (*compar)(const FTSENT **, const FTSENT **) = fts_alphasort;
	if (args.sort_time)
		compar = fts_mtimesort;
	if (args.show_access_time)
		compar = fts_atimesort;
	int nprint = get_nprint(fts_open(pathav, FTS_PHYSICAL | (FTS_SEEDOT * args.all) | FTS_XDEV, compar), &args);
	if (nprint < 0)
		return 1;
	FTS *fts = fts_open(pathav, FTS_PHYSICAL | (FTS_SEEDOT * args.all) | FTS_XDEV, compar);
	int first = 1;
	for (FTSENT *ent = fts_read(fts); ent != NULL; ent = fts_read(fts)) {
		if (ent->fts_info == FTS_DP)
			continue ;
		const char *name = ent->fts_name[0] != 0 ? ent->fts_name : ent->fts_accpath;
		if (ent->fts_level > 0 && (!args.recursive
				|| (!args.all && name[0] == '.'))) {
			fts_set(fts, ent, FTS_SKIP);
			continue ;
		}
		switch (ent->fts_info) {
			case FTS_ERR:
			case FTS_NS:
			case FTS_NSOK:
			case FTS_DNR:
				switch (ent->fts_errno) {
					case EACCES:
						fprintf(stderr, "%s: cannot open directory '%s': %s\n", prog, name, strerror(ent->fts_errno)); break;
					case ENOENT:
					default:
						fprintf(stderr, "%s: cannot access '%s': %s\n", prog, name, strerror(ent->fts_errno)); break;
				}
			continue ;

		}
		if (ent->fts_info == FTS_D || ent->fts_level == 0) {
			if (nprint > 1 && !first)
					fprintf(stdout, "\n");
			if (!fts_print_dir(fts_children(fts, 0), &args, nprint > 1))
				continue;
			else if (!args.long_fmt && !args.long_no_owner)
				fprintf(stdout, "\n");
			first = 0;
		} else if (ent->fts_info == FTS_ERR) {
			fprintf(stderr, "%s: %s\n", prog, strerror(ent->fts_errno));
		}
	}
	fts_close(fts);
	return 0;
}
