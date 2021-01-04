/*
 *  key/value hash table.
 */

#include <assert.h>

#include "hash-table.h"
#include "log.h"
#include "mem.h"

struct hash_table *hash_table_init(unsigned int count)
{
	struct hash_table *ht;
	unsigned int i;

	if (count < 1) {
		on_error("too small.\n");
	}

	ht = mem_alloc_zero(sizeof(*ht) + count * sizeof(ht->array[0]));

	list_init(&ht->extras, "ht extras");

	ht->count = count;

	for (i = 0; i < ht->count; i++) {
		list_init(&ht->array[i], "ht list");
	}

	return ht;
}

void hash_table_entry_init(struct hash_table_entry *hte, struct list *list,
	unsigned long key, void *data)
{
	hte->my_list = list;
	hte->key = key;
	hte->data = data;
	list_entry_init(hte->my_list, &hte->list_entry);
}

void hash_table_insert(struct hash_table *ht, unsigned int index,
	struct hash_table_entry *hte)
{
	//debug("index-%u: %lu => %p\n", index, hte->key, hte->data);

	assert(index < ht->count);
	assert(hte->list_entry.list_mtx);

	list_add_tail(&ht->array[index], &hte->list_entry);
}

void hash_table_insert_extra(struct hash_table *ht,
	struct hash_table_entry *hte)
{
	//debug("%u@%lu => %p\n", hte->data);

	assert(hte->list_entry.list_mtx);

	list_add_tail(&ht->extras, &hte->list_entry);
}

void hash_table_remove(struct hash_table_entry *hte)
{
	list_remove(&hte->list_entry);
}

struct hash_table_entry *hash_table_find_first(struct hash_table *ht,
	unsigned int index, unsigned long key)
{
	struct hash_table_entry *hte;

	assert(index < ht->count);

	list_for_each(&ht->array[index], hte, list_entry) {
		if (key == hte->key) {
			debug("found    %u@%lu => %p\n", index, key,
				hte->data);
			return hte;
		} else {
			debug("check    %u@%lu => %p\n", index, key,
				hte->data);
		}
	}

	debug("not found %u@%lu\n", index, key);
	return NULL;
}

struct hash_table_entry *hash_table_find_next(
	struct hash_table_entry *hte)
{
	unsigned long key = hte->key;

	list_for_each_continue(hte->my_list, hte, list_entry) {
		if (key == hte->key) {
			debug("found    @%lu => %p\n", key, hte->data);
			return hte;
		} else {
			debug("check    @%lu => %p\n", key, hte->data);
		}
	}

	debug("not found %lu\n", key);
	return NULL;
}

int hash_table_for_each_list(const struct hash_table *ht,
	hash_table_for_each_list_cb cb_fn, void *cb_data)
{
	unsigned int i;
	int result = 0;

	for (i = 0; i <  ht->count; i++) {
		//debug("list [%u]\n", i);
		result = cb_fn(cb_data, &(ht->array[i]));
		if (result) {
			break;
		}
	}
	return result;
}
