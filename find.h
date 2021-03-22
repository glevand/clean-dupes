/*
 *  Find files.
 */

#if !defined(_FIND_FILES_H)
#define _FIND_FILES_H

#include "digest.h"
#include "hash-table.h"
#include "work-queue.h"

struct file_data {
	struct digest digest;
	bool matched;
	size_t name_len;
	char name[];
};

int find_files(struct work_queue *wq, struct hash_table *ht,
	bool (*check_for_signals)(void), const char *parent_path);
void file_table_entry_clean(struct hash_table_entry *hte);
unsigned long file_count(struct hash_table *ht);

#endif /* _FIND_FILES_H */
