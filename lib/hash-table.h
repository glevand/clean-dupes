/*
 *  key/value hash table.
 */

#if !defined(_LIB_HASH_TABLE)
#define _LIB_HASH_TABLE

#include "list.h"

struct hash_table_entry {
	struct list_entry list_entry;
	unsigned long key;
	void *data;
	struct list *my_list;
};

struct hash_table {
	unsigned int count;
	struct list array[];
};

struct hash_table *hash_table_init(unsigned int count);
void hash_table_entry_init(struct hash_table_entry *hte, struct list *list,
	unsigned long key, void *data);

void hash_table_insert(struct hash_table *ht, unsigned int index,
	struct hash_table_entry *hte);
void hash_table_remove(struct hash_table_entry *hte);

struct hash_table_entry *hash_table_find_first(struct hash_table *ht,
	unsigned int index, unsigned long key);
struct hash_table_entry *hash_table_find_next(
	struct hash_table_entry *hte);

typedef int (*hash_table_for_each_list_cb)(void *cb_data,
	const struct list *list);

int hash_table_for_each_list(const struct hash_table *ht,
	hash_table_for_each_list_cb cb_fn, void *cb_data);

static inline unsigned int hash_table_index(const struct hash_table *ht,
	unsigned long key)
{
	return (ht->count - 1) & (unsigned int)key;
}

#endif /* _LIB_HASH_TABLE */
