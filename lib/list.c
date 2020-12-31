/*
 *  Doubly linked list.
 */

#include <assert.h>
#include <stdlib.h>

#include "list.h"
#include "log.h"

#if defined (DEBUG_LIST)
# define list_debug log_raw
#else
# define list_debug(_args...) while(0) {_debug(__func__, __LINE__, _args);}
#endif

void list_init(struct list *list, const char *name)
{
	int result;

	result = mtx_init(&list->mtx, mtx_plain);

	if (result) {
		fprintf(stderr, "%s: error: mxt_init: %d\n", __func__, result);
		exit(EXIT_FAILURE);
	}

	list->head.next = &list->head;
	list->head.prev = &list->head;
	list->head.list_mtx = &list->mtx;
	list_debug("'%s': %p %p\n", name, list, &list->mtx);
}

void list_insert_before(struct list_entry *next, struct list_entry *le)
{
	list_debug("%p %p >\n", le, le->list_mtx);

	assert(le->list_mtx);

	list_lock(le->list_mtx);
	le->next = next;
	le->prev = next->prev;
	next->prev->next = le;
	next->prev = le;
	list_unlock(le->list_mtx);
	list_debug("%p %p <\n", le, le->list_mtx);
}

void list_insert_after(struct list_entry *prev, struct list_entry *le)
{
	list_debug("%p %p >\n", le, le->list_mtx);

	assert(le->list_mtx);

	list_lock(le->list_mtx);
	le->next = prev->next;
	le->prev = prev;
	prev->next->prev = le;
	prev->next = le;
	list_unlock(le->list_mtx);
	list_debug("%p %p <\n", le, le->list_mtx);
}

void list_add_tail(struct list *list, struct list_entry *le)
{
	list_debug(">\n");
	list_entry_init(list, le);
	list_insert_before(&list->head, le);
	list_debug("<\n");
}

#if 0 
static void __attribute__ ((unused)) list_remove_no_lock(struct list_entry *le)
{
	list_debug("%p %p >\n", le, le->list_mtx);
	le->next->prev = le->prev;
	le->prev->next = le->next;
	list_debug("%p %p <\n", le, le->list_mtx);
}
#endif

void list_remove(struct list_entry *le)
{
	list_debug("%p %p >\n", le, le->list_mtx);
	list_lock(le->list_mtx);
	le->next->prev = le->prev;
	le->prev->next = le->next;
	list_unlock(le->list_mtx);
	list_debug("%p %p <\n", le, le->list_mtx);
}

struct list_entry *list_get_first(struct list *list)
{
	struct list_entry *le;

	list_debug("%p %p\n", list, &list->mtx);

	list_lock(&list->mtx);

	if (list->head.next == &list->head) {
		list_debug("NULL\n");
		le = NULL;
		goto exit;
	}

	for (le = list->head.next; le != &list->head; le = le->next) {
		if (!le->in_use) {
			le->in_use = true;
			list_debug("%p %p %p %p\n", list, &list->mtx, le,
				le->list_mtx);
			break;
		}
	}

exit:
	list_unlock(&list->mtx);
	return le;
}

unsigned int list_item_count(const struct list *list)
{
	unsigned int count;
	struct list_entry *le;

	list_lock((mtx_t *)&list->mtx);
	for (count = 0, le = list->head.next; le != &list->head;
	     count++, le = le->next) {
		(void)0;
	}
	list_unlock((mtx_t *)&list->mtx);

	//debug("count = %u\n", count);
	return count;
}

bool list_is_empty(const struct list *list)
{
	bool result;

	list_lock((mtx_t *)&list->mtx);
	result = list->head.next == &list->head;
	list_unlock((mtx_t *)&list->mtx);

	return result;
}

void _list_lock(mtx_t *list_mtx)
{
	int result;

	result = mtx_lock(list_mtx);

	if (result == thrd_error) {
		log("ERROR: mtx_lock: %d\n", result);
		exit(EXIT_FAILURE);
	}
}

void _list_unlock(mtx_t *list_mtx)
{
	int result;

	result = mtx_unlock(list_mtx);

	if (result == thrd_error) {
		log("ERROR: mtx_lock: %d\n", result);
		exit(EXIT_FAILURE);
	}
}

void __attribute__ ((unused)) _list_lock_debug(mtx_t *list_mtx,
	const char *func, int line)
{
	list_debug("%s:%d: list lock => %p\n", func, line, list_mtx);
	_list_lock(list_mtx);
	list_debug("%s:%d: list lock <= %p\n", func, line, list_mtx);
}

void __attribute__ ((unused)) _list_unlock_debug(mtx_t *list_mtx,
	const char *func, int line)
{
	list_debug("%s:%d: list unlock => %p\n", func, line, list_mtx);
	_list_unlock(list_mtx);
	list_debug("%s:%d: list unlock <= %p\n", func, line, list_mtx);
}


