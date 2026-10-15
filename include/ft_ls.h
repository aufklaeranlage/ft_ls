#ifndef FT_LS_H
# define FT_LS_H

struct args {
	/* -l : long format; show owner  */
	unsigned int long_fmt: 1;
	/* -g : same as long format, but doesn't show owners; overwrites -l */
	unsigned int long_no_owner : 1;
	/* -R : recursive; also run on directories inside directories */
	unsigned int recursive : 1;
	/* -a : show hidden entries */
	unsigned int all : 1;
	/* -r : reverse sort */
	unsigned int reverse : 1;
	/* -t : sort by time */
	unsigned int sort_time : 1;
	/* -u : With -lt or without -l sort by at; with -l sort by name */
	unsigned int show_access_time : 1;
	/* -d : list only directories given via cl */
	unsigned int list_directories: 1;
};

#endif /* !FT_LS_H */
