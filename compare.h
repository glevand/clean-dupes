/*
 *  Compare files.
 */

#if !defined(_FIND_DUPES_H)
#define _FIND_DUPES_H

#include <stdio.h>

#include "hash-table.h"
#include "work-queue.h"

struct compare_file_pointers {
	FILE *dupes;
	FILE *unique;
};

struct compare_counts {
	unsigned int total;
	unsigned int dupes;
	unsigned int unique;
};

void compare_files(struct work_queue *wq, struct hash_table *ht,
	bool (*check_for_signals)(void), struct compare_file_pointers *fps);

#endif /* _FIND_DUPES_H */
