#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "mem.h"
#include "util.h"

unsigned int to_unsigned(const char *str)
{
	const char *p;
	unsigned long u;

	for (p = str; *p; p++) {
		if (!isdigit(*p)) {
			log("isdigit failed: '%s'\n", str);
			return 0L;
		}
	}

	u = strtoul(str, NULL, 10);

	if (u == ULONG_MAX) {
		log("strtoul '%s' failed: %s\n", str, strerror(errno));
		return 0L;
	}

	if (u > UINT_MAX) {
		log("too big: %lu\n", u);
		return UINT_MAX;
	}

	return (unsigned int)u;
}

void print_current_time(FILE *fp)
{
	char str[256];
	time_t t;
	struct tm *tm;
	size_t result;

	t = time(NULL);
	tm = localtime(&t);

	if (!tm) {
		log("localtime failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	result = strftime(str, sizeof(str), "%a %d %b %Y %r %Z", tm);

	if (!result) {
		log("strftime failed.\n");
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "%s", str);
}

bool test_for_dots(const char *d_name)
{
	return (d_name[0] == '.'
		&& (d_name[1] == 0 || (d_name[1] == '.' && d_name[2] == 0)));
}

char *make_sub_path(const char *parent_path, unsigned int parent_len,
	const char *sub_name)
{
	unsigned int sub_len;
	char *sub_path;

	sub_len = strlen(sub_name);
	sub_path = mem_alloc(parent_len + sub_len + 2);

	//memset(sub_path, '*', parent_len + sub_len + 2);
	//sub_path[parent_len + sub_len + 1] = 0;
	//debug("@%s@\n", sub_path);

	memcpy(sub_path, parent_path, parent_len);
	//debug("@%s@\n", sub_path);
	sub_path[parent_len] = '/';
	//debug("@%s@\n", sub_path);
	memcpy(sub_path + parent_len + 1, sub_name, sub_len + 1);
	//debug("@%s@\n", sub_path);

	return sub_path;
}

void check_exists(const char *file)
{
	if (access(file, R_OK)) {
		log("ERROR: access '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
