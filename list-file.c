/*
 *  List file.
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif


#include <assert.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "mem.h"

#include "find.h"
#include "list-file.h"

struct list_file_data {
	FILE *list_fp;
	unsigned int file_counter;
	unsigned int list_entry_max;
};

static bool list_file_print_cb(void *cb_data, const struct list *list)
{
	struct list_file_data *pfl_data = cb_data;
	struct hash_table_entry *hte;
	unsigned int counter = 0;

	list_for_each(list, hte, list_entry) {
		struct file_data *data;

		counter++;
		pfl_data->file_counter++;

		data = (struct file_data *)hte->data;
		fprintf(pfl_data->list_fp, "%lu %s\n", hte->key, data->name);
	}

	pfl_data->list_entry_max = (counter > pfl_data->list_entry_max)
		? counter : pfl_data->list_entry_max;

	return false;
}

FILE *list_file_open(const char *parent_dir, const char *file)
{
	char *path;
	FILE *fp;

	assert(parent_dir);

	path = mem_strdupcat(parent_dir, file);

	fp = fopen(path, "w");

	if (!fp) {
		fprintf(stderr, "ERROR: fopen '%s' failed: %s\n", path,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	return fp;
}

bool list_file_print(const struct hash_table *ht, FILE *list_fp)
{
	struct list_file_data pfl_data = {
		.list_fp = list_fp,
		.file_counter = 0,
		.list_entry_max = 0,
	};
	int result;

	result = hash_table_for_each_list(ht, list_file_print_cb, &pfl_data);

	if (result) {
		return result;
	}

	debug("file_counter = %u\n", pfl_data.file_counter);
	debug("list_entry_max = %u\n", pfl_data.list_entry_max);
	return false;
}
