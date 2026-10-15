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

static int open_dir(DIR **dir, const char *name)
{
	*dir = opendir(name);
	if (*dir == NULL) {
		int err = errno;
		switch (err) {
			case EACCES:
				fprintf(stderr, "%s: cannot open directory '%s': %s\n", prog, name, strerror(err));
				return 0;
			case ENOENT:
			case ENOTDIR:
			default:
				fprintf(stderr, "%s: cannot access '%s': %s\n", prog, name, strerror(err));
				return 0;
		}
	}
	return 1;
}

int main(int ac, char *av[])
{
	prog = av[0];
	struct args args;
	int pos = read_args(&args, ac, av);
	if (pos < 0)
		return 1;
	char *dirname = av[pos] == NULL ? "." : av[pos];
	int pfile = ac - pos > 1;
	for (; dirname; dirname = av[++pos]) {
		DIR *dir;
		if (!open_dir(&dir, dirname))
			continue;
		if (pfile)
			fprintf(stdout, "%s:\n", dirname);
		closedir(dir);
		if (pos >= ac)
			break;
	}
	return 0;
}
