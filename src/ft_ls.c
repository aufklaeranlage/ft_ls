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
#include <stdlib.h>

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

static int ndigits(long num, unsigned int base)
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

struct print_fmt_len {
	int acl;
	int nlink;
	int user;
	int group;
	int bsize;
	size_t blocks;
};

static int get_print_fmt_len(struct print_fmt_len *pfl, FTSENT *head, struct args *args)
{
	*pfl = (struct print_fmt_len) { };
	int digits;
	struct passwd *pwd;
	struct group *grp;
	const char *name;
	pfl->blocks = 0;
	for (FTSENT *node = head; node != NULL; node = node->fts_link) {
		char *path = make_path(node->fts_path, node->fts_accpath);
		int len;
		name = node->fts_name; 
		if (!args->all && name[0] == '.')
			continue ;
		if (has_acl(path))
			pfl->acl = 1;
		free(path);
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
		pfl->blocks += node->fts_statp->st_blocks;
	}
	pfl->blocks /= 2;
	return 1;
}

static void set_username(char **user, uid_t uid)
{
	struct passwd *pwd;
	pwd = getpwuid(uid);
	if (!pwd)
		return;
	*user = pwd->pw_name; 
}

static void set_groupname(char **group, gid_t gid)
{
	struct group *grp;
	grp = getgrgid(gid);
	if (!grp)
		return;
	*group = grp->gr_name; 
}

static void set_dates(char **date1, char **date2, struct stat *st, int atime)
{
	const time_t old_file_seconds = 15778476;
	char datestr[2][256];
	time_t curtime;
	time(&curtime);
	int dslen;
	struct tm *tm = atime ?
		localtime(&st->st_atime) :
		localtime(&st->st_mtime);
	dslen = strftime(datestr[0], sizeof(datestr[0]), "%b %e", tm);
	if (dslen <= 0)
		return;
	*date1 = datestr[0];
	dslen = strftime(datestr[1], sizeof(datestr[1]),
			(curtime - (atime ? st->st_atime : st->st_mtime) >= old_file_seconds ? "%Y" : "%H:%M"),
			tm);
	if (dslen <= 0)
		return;
	*date2 = datestr[1];
}

static int fts_print_ent(FTSENT *ent, struct args *args, struct print_fmt_len *pfl)
{
	struct stat *st;
	const char *permstr = "-?????????";
	char *path = make_path(ent->fts_path, ent->fts_accpath),
			 *acl = "",
			 *user = "?",
			 *group = "?",
			 *date1 = "?",
			 *date2 = "?",
			 *file = ent->fts_name,
			 symln[256] = " -> ";
	int no_stat = 0;
	switch (ent->fts_info) {
		case FTS_ERR:
		case FTS_NS:
		case FTS_NSOK:
			no_stat = 1;
			break;
		case FTS_SL:
		case FTS_DOT:
		case FTS_DP:
		case FTS_D:
		case FTS_F:
		default:
			st = ent->fts_statp;
			permstr = perm_str(st->st_mode);
			acl = has_acl(path) ? "+" : "";
			set_username(&user, st->st_uid);
			set_groupname(&group, st->st_gid);
			set_dates(&date1, &date2, st, args->show_access_time);
			if (ent->fts_info == FTS_SL) {
				if (readlink(path, symln + 4, sizeof(symln) - 4) < 0) {
					symln[0] = '?'; symln[1] = 0;
				}
			}
	}
	fprintf(stdout, "%s%*s %*ju %*s %*s %*jd %*s %*s %s%s\n",
			permstr,
			pfl->acl, acl,
			pfl->nlink, no_stat ? 0 : (uintmax_t) st->st_nlink,
			!args->long_no_owner ? 0 : pfl->user , !args->long_no_owner ? user : "", 
			pfl->group, group, 
			pfl->bsize, no_stat ? 0 : (intmax_t) st->st_size,
			6, date1,
			5, date2,
			file,
			ent->fts_info == FTS_SL ? symln : "");
	free(path);
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
	if (args->long_fmt || args->long_no_owner)
		fprintf(stdout, "total %lu\n", pfl.blocks);
	int first = 1;
	for (FTSENT *node = head; node != NULL; node = node->fts_link) {
		name = node->fts_name; 
		if (!args->all && name[0] == '.')
			continue ;
		if (!args->long_fmt && !args->long_no_owner) {
			fprintf(stdout, "%s%s", first ? "" : "  ", name);
		} else {
			switch (node->fts_info) {
				case FTS_DOT:
					if (!args->all)
				case FTS_DP:
						continue;
				default:
						fts_print_ent(node, args, &pfl);
			}
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
	int (*compar_arr[3][2])(const FTSENT **, const FTSENT **) = {
		{fts_alphasort, fts_ralphasort},
		{fts_mtimesort, fts_rmtimesort},
		{fts_atimesort, fts_ratimesort}
	};
	int (*compar)(const FTSENT **, const FTSENT **) = compar_arr[0][args.reverse];
	if (args.sort_time) {
		if (args.show_access_time)
			compar = compar_arr[2][args.reverse];
		else
			compar = compar_arr[1][args.reverse];
	}
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
