/*
 *  Find files.
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

#include "log.h"
#include "mem.h"
#include "util.h"

#include "find.h"

struct file_table_entry {
	struct hash_table_entry hte;
	struct file_data file_data;
};

static struct file_table_entry *hte_to_fte(struct hash_table_entry *hte)
{
	return container_of(hte, struct file_table_entry, hte);
}

void file_table_entry_clean(struct hash_table_entry *hte)
{
	struct file_table_entry *fte = hte_to_fte(hte);

	list_remove(&hte->list_entry);

	mem_free(fte);
}

static struct hash_table_entry *ht_entry_init(const char *file_name,
	unsigned long file_size, struct list *list)
{
	struct file_table_entry *fte;
	unsigned int len = strlen(file_name);

	fte = mem_alloc_zero(sizeof(*fte) + len + 1);

	fte->file_data.name_len = len;
	memcpy(fte->file_data.name, file_name, len);

	//debug("%p: '%s'\n", list, file_name);
	hash_table_entry_init(&fte->hte, list, file_size, &fte->file_data);

	return &fte->hte;
}

static long int __attribute__((unused)) get_file_size_ftell(const char *file)
{
	FILE *fp;
	long int size;
	int result;

	fp = fopen(file, "r");

	if (!fp) {
		log("ERROR: fopen '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	result = fseek(fp, 0, SEEK_END);

	if (result) {
		log("ERROR: fseek '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	size = ftell(fp);

	if (size == -1) {
		log("ERROR: ftell '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	fclose(fp);

	return size;
}

static off_t get_file_size_stat64(const char *file)
{
	struct stat64 st;
	int result;

	result = stat64(file, &st);

	if (result) {
		log("ERROR: stat '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (st.st_size < 0) {
		log("ERROR: stat '%s' failed: %s\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return st.st_size;
}

static void process_file(const char *file, struct hash_table *ht)
{
	struct hash_table_entry *hte;
	off_t size;

	size = get_file_size_stat64(file);

	if (size) {
		unsigned int index = hash_table_index(ht, (long int)size);

		hte = ht_entry_init(file, (long int)size, &ht->array[index]);
		hash_table_insert(ht, index, hte);
		//debug("index-%u: size = %ld, %s\n", index, (long int)size, file);
	} else {
		hte = ht_entry_init(file, (long int)size, &ht->extras);
		hash_table_insert_extra(ht, hte);
	}
}

struct find_files_cb_data {
	struct work_queue *wq;
	struct hash_table *ht;
	bool (*check_for_signals)(void);
	char *sub_path;
};

static int find_files_cb(struct work_item *wi)
{
	struct find_files_cb_data *cbd = wi->cb_data;
	int result;
	//char *dup;

	//dup = strdup(cbd->sub_path);
	//debug("> '%s'\n", cbd->sub_path);

	result = find_files(cbd->wq, cbd->ht, cbd->check_for_signals,
		cbd->sub_path);

	assert(wi->list_entry.in_use);

	list_remove(&wi->list_entry);

	mem_free(wi);

	//debug("< '%s'\n", dup);
	//free(dup);
	return result;
}

static void find_files_queue_work(unsigned int id, struct work_queue *wq,
	struct hash_table *ht, bool (*check_for_signals)(void),
	const char *sub_path)
{
	struct find_files_cb_data *cbd;
	struct work_item *wi;
	unsigned int sub_path_len = strlen(sub_path) + 1;

	wi = mem_alloc_zero(sizeof(*wi) + sizeof(*cbd) + sub_path_len);

	cbd = (void*)(wi + 1);
	cbd->sub_path = (void*)(cbd + 1);
	cbd->wq = wq;
	cbd->ht = ht;
	cbd->check_for_signals = check_for_signals;
	memcpy(cbd->sub_path, sub_path, sub_path_len);

	wi->id = id;
	wi->cb = find_files_cb;
	wi->cb_data = cbd;

	work_queue_add_item(wq, wi);
}

int find_files(struct work_queue *wq, struct hash_table *ht,
	bool (*check_for_signals)(void), const char *parent_path)
{
	unsigned int parent_len = 0;
	struct dirent *de;
	unsigned int id;
	int result = 0;
	DIR *dp;

	//debug("> '%s'\n", parent_path);

	dp = opendir(parent_path);

	if (!dp) {
		log("ERROR: opendir '%s' failed: %s\n", parent_path,
			strerror(errno));
		exit(EXIT_FAILURE);

	}

	for (id = 0; ; id++) {
		if (check_for_signals()) {
			//debug("exit on signal\n");
			result = -1;
			goto exit;
		}

		de = readdir(dp);

		if (!de) {
			//debug("done:  '%s'\n", parent_path);
			break;
		}

		//debug("d_name = '%s'\n", de->d_name);

		switch (de->d_type) {
		case DT_DIR: {
			char *sub_path;

			if (test_for_dots(de->d_name)) {
				continue;
			}
			//debug("DIR:  '%s'\n", de->d_name);

			if (!parent_len) {
				parent_len = strlen(parent_path);
			}

			sub_path = make_sub_path(parent_path, parent_len,
				de->d_name);

			//debug("DT_DIR: %s\n", sub_path);
			find_files_queue_work(id, wq, ht, check_for_signals,
				sub_path);
			mem_free(sub_path);

			break;
		}
		case DT_REG: {
			char *sub_path;

			if (!parent_len) {
				parent_len = strlen(parent_path);
			}
			sub_path = make_sub_path(parent_path, parent_len,
				de->d_name);

			//debug("DT_REG: %s\n", sub_path);
			process_file(sub_path, ht);
			mem_free(sub_path);

			break;
		}
		default:
			break;
		}
	}

exit:
	closedir(dp);
	//debug("< '%s'\n", parent_path);
	return result;
}

unsigned long file_count(struct hash_table *ht)
{
	unsigned long count;
	unsigned int i;

	count = list_item_count(&ht->extras);

	for (i = 0; i < ht->count; i++) {
		count += list_item_count(&(ht->array[i]));
	}
	return count;
}
