#include "utils.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

const char *prog = "ft_ls";

static void print_usage(void)
{
	
}

int main(int ac, char *av[])
{
	prog = av[0];
	if (ac < 2)
		return 1;
	char *dirname = av[1];
	DIR *dir = opendir(dirname);
	if (dir == NULL) {
		int err = errno;
		switch (err) {
			case EACCES:
				fprintf(stderr, "%s: cannot open directory '%s': %s\n", prog, dirname, strerror(err));
				return 1;
			case ENOENT:
			case ENOTDIR:
			default:
				fprintf(stderr, "%s: cannot access '%s': %s\n", prog, dirname, strerror(err));
				return 1;
		}
	}
	closedir(dir);
	return 0;
}
