/*
 *  List file.
 */

#if !defined(_LIST_FILE_H)
#define _LIST_FILE_H

#include <stdio.h>

#include "hash-table.h"

FILE *list_file_open(const char *parent_dir, const char *file);
bool list_file_print(const struct hash_table *ht, FILE *list_fp);

#endif /* _LIST_FILE_H */
