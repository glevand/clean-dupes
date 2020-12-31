/*
 *  utilities.
 */

#if !defined(_LIB_UTIL_H)
#define _LIB_UTIL_H

#include <stdbool.h>
#include <stdio.h>

const char *version_string;

#define build_assert(_x) do {(void)sizeof(char[(_x) ? 1 : -1]);} while (0)
unsigned int to_unsigned(const char *str);
void print_current_time(FILE *fp);

bool test_for_dots(const char *d_name);
char *make_sub_path(const char *parent_path, unsigned int parent_len,
	const char *sub_name);
void check_exists(const char *file);

#endif /* _LIB_UTIL_H */
