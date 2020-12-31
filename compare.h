/*
 *  Compare files.
 */

#if !defined(_FIND_DUPES_H)
#define _FIND_DUPES_H

#include <stdio.h>

#include "hash-table.h"
#include "work-queue.h"

void compare_files(struct work_queue *wq, struct hash_table *ht,
	bool (*check_for_signals)(void), FILE *dupes_fp, FILE *unique_fp);

struct compare_result {
	unsigned int total_count;
	unsigned int unique_count;
	unsigned int dupe_count;
};

#endif /* _FIND_DUPES_H */
