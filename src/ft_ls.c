#include "ft_ls.h"
#include "utils.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

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

// static int open_dir(DIR **dir, const char *name)
// {
// 	*dir = opendir(name);
// 	if (*dir == NULL) {
// 		int err = errno;
// 		switch (err) {
// 			case EACCES:
// 				fprintf(stderr, "%s: cannot open directory '%s': %s\n", prog, name, strerror(err));
// 				return 0;
// 			case ENOENT:
// 			case ENOTDIR:
// 			default:
// 				fprintf(stderr, "%s: cannot access '%s': %s\n", prog, name, strerror(err));
// 				return 0;
// 		}
// 	}
// 	return 1;
// }

extern const char *NS_PERM_STR;

static int fts_print_dir(FTSENT *ent, struct args *args)
{
	if (!ent)
		return 0;
	const char *name;
	const char *permstr;
	for (FTSENT *node = ent; node != NULL; node = node->fts_link) {
		// if (pfile)
		// 	fprintf(stdout, "%s:\n", name);
		name = node->fts_name; 
		if (!args->all && name[0] == '.')
			continue ;
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
		fprintf(stdout, "%s %s\n", permstr, name);
	}
	return 1;
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
	int pfile = ac - pos > 1;
	int (*compar)(const FTSENT **, const FTSENT **) = fts_alphasort;
	if (args.sort_time)
		compar = fts_mtimesort;
	if (args.show_access_time)
		compar = fts_atimesort;
	FTS *fts = fts_open(pathav, FTS_PHYSICAL | (FTS_SEEDOT * args.all) | FTS_XDEV, compar);
	int first = 1;
	for (FTSENT *ent = fts_read(fts); ent != NULL; ent = fts_read(fts)) {
		if (ent->fts_info & FTS_DP)
			continue ;
		if (ent->fts_level > 0 && !args.recursive) {
			fts_set(fts, ent, FTS_SKIP);
			continue ;
		}
		const char *name = ent->fts_info & FTS_D ? ent->fts_path : ent->fts_name;
		if (pfile) {
			if (!first)
				fprintf(stdout, "\n");
			else
				first = 0;
			fprintf(stdout, "%s:\n", name);
		}
		fts_print_dir(fts_children(fts, 0), &args);
	}
	fts_close(fts);
	return 0;
}
