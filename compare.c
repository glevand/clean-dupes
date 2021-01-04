/*
 *  Compare files.
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
#include "util.h"

#include "compare.h"
#include "find.h"

//#define DEBUG_COMPARE

#if defined(DEBUG_COMPARE)
# define cp_debug(_args...) do {_debug(__func__, __LINE__, _args);} while(0)
#else
# define cp_debug(_args...) while(0) {_debug(__func__, __LINE__, _args);}
#endif

struct compare_files_cb_data {
	const struct list *ht_list;
	bool (*check_for_signals)(void);
	struct compare_file_pointers *fps;
};

struct dupe_buffer {
	char *buf;
	size_t len;
};

static void dupe_buffer_write_first(struct dupe_buffer* db,
	struct file_data *data_1)
{
	static const char prefix[] = {'[', '1', ']', ' '};

	db->buf = realloc(db->buf, db->len + sizeof(prefix) + data_1->name_len
		+ 1);

	memcpy(db->buf + db->len, prefix, sizeof(prefix));
	memcpy(db->buf + db->len + sizeof(prefix), data_1->name,
		data_1->name_len);

	*(db->buf + db->len + sizeof(prefix) + data_1->name_len) = '\n';

	db->len += sizeof(prefix) + data_1->name_len + 1;
}

static void dupe_buffer_write_match(struct dupe_buffer* db,
	struct file_data *data_2, unsigned int match_counter)
{
	static const unsigned int prefix_len = sizeof("[4294967295] ") - 1;

	db->buf = realloc(db->buf, db->len + prefix_len + data_2->name_len + 2);

	db->len += sprintf(db->buf + db->len, "[%u] %s\n", match_counter + 1,
		data_2->name);
}

static int compare_files_cb(struct work_item *wi)
{
	struct compare_files_cb_data *cbd = wi->cb_data;
	struct hash_table_entry *hte_1;
	struct hash_table_entry *hte_safe;
	struct compare_counts *compare_result = wi->result;
	int result = 0;
	unsigned int i;

	if (cbd->check_for_signals()) {
		result = -1;
		goto exit;
	}

	i = 1;
	cp_debug("=== wi-%u ===\n", wi->id);
	list_for_each(cbd->ht_list, hte_1, list_entry) {
		struct file_data *data = (struct file_data *)hte_1->data;
		cp_debug("hte-%u = %p: key = %lu, %s\n", i++, hte_1,
			hte_1->key, data->name);
	}
	cp_debug("============\n");

	list_for_each_safe(cbd->ht_list, hte_1, hte_safe, list_entry) {
		struct hash_table_entry *hte_2;
		unsigned int match_counter = 0;
		struct dupe_buffer d_buf = {
			.buf = NULL,
			.len = 0,
		};
		struct file_data *data_1;

		compare_result->total++;

		data_1 = (struct file_data *)hte_1->data;

		cp_debug("--------\n");
		cp_debug("       hte_1 = %p: key = %lu, %s\n", hte_1,
			hte_1->key, data_1->name);

		i = 0;
		hte_2 = hte_1;
		list_for_each_continue(cbd->ht_list, hte_2, list_entry) {
			struct file_data *data_2;

			i++;
			data_2 = (struct file_data *)hte_2->data;

			if (data_2->matched) {
				cp_debug("skip: hte_2.%u = %p: key = %lu, %s\n",
					i, hte_2, hte_2->key, data_2->name);
				continue;
			}

			if (hte_1->key != hte_2->key) {
				cp_debug("      hte_2.%u = %p: key = %lu, %s\n",
					i, hte_2, hte_2->key, data_2->name);
			} else {
				if (md5sum_empty(&data_1->md5sum)) {
					md5sum_file(&data_1->md5sum,
						data_1->name);
				}
				if (md5sum_empty(&data_2->md5sum)) {
					md5sum_file(&data_2->md5sum,
						data_2->name);
				}
				if (! md5sum_compare(&data_1->md5sum,
					&data_2->md5sum)) {
					cp_debug("key:  hte_2.%u = %p: key = %lu, %s\n",
						i, hte_2, hte_2->key,
						data_2->name);
				} else {
					cp_debug("sum:  hte_2.%u = %p: key = %lu, %s\n",
						i, hte_2, hte_2->key,
						data_2->name);
					if (!match_counter) {
						dupe_buffer_write_first(&d_buf,
							data_1);
					}
					data_2->matched = true;
					match_counter++;
					dupe_buffer_write_match(&d_buf, data_2,
						match_counter);
				}
			}
		}

		if (cbd->check_for_signals()) {
			//debug("exit on signal\n");
			result = -1;
			goto exit;
		}

		if (!match_counter) {
			assert(!d_buf.buf);
			compare_result->unique++;
			fprintf(cbd->fps->unique, "%s\n", data_1->name);
		} else {
			size_t result;

			cp_debug("found %u dupes\n", match_counter);

			compare_result->dupes += match_counter;

			assert(d_buf.buf);
			*(d_buf.buf + d_buf.len) = '\n';
			d_buf.len++;

			result = fwrite(d_buf.buf, 1, d_buf.len,
				cbd->fps->dupes);

			if (result != d_buf.len) {
				log("ERROR: fwrite failed: %s\n",
					strerror(errno));
				assert(0);
				exit(EXIT_FAILURE);
			}

			free(d_buf.buf);
		}

		file_table_entry_clean(hte_1);
	}

exit:
	list_for_each(cbd->ht_list, hte_1, list_entry) {
		struct file_data *data = (struct file_data *)hte_1->data;
		cp_debug("hte-%u = %p: key = %lu, %s\n", i++, hte_1,
			hte_1->key, data->name);
	}
	work_queue_finish_item(wi);
	return result;
}


static void compare_files_queue_work(unsigned int id, struct work_queue *wq,
	bool (*check_for_signals)(void), struct compare_file_pointers *fps,
	const struct list *ht_list)
{
	struct compare_files_cb_data *cbd;
	struct work_item *wi;

	wi = mem_alloc_zero(sizeof(*wi) + sizeof(*cbd)
		+ sizeof(struct compare_counts));

	cbd = wi->cb_data = (void*)(wi + 1);
	cbd->fps = fps;
	cbd->check_for_signals = check_for_signals;
	cbd->ht_list = ht_list;

	wi->id = id;
	wi->cb = compare_files_cb;
	wi->result = (void*)(cbd + 1);

	//debug("> id = %u, list = %p, %p\n", wi->id, cbd->ht_list, &wq->ready_list);
	work_queue_add_item(wq, wi);
}

void compare_files(struct work_queue *wq, struct hash_table *ht,
	bool (*check_for_signals)(void), struct compare_file_pointers *fps)
{
	unsigned int i;

	for (i = 0; i < ht->count; i++) {
		//debug("queue list[%u]\n", i);
		compare_files_queue_work(i, wq, check_for_signals, fps,
			&(ht->array[i]));
	}
}
