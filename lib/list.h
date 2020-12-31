/*
 *  Doubly linked list.
 */

#if !defined(_LIB_LIST_H)
#define _LIB_LIST_H

#include <threads.h>
#include <stdbool.h>

#if !defined(offsetof)
#define offsetof(_type, _member) ((size_t) &((_type *)0)->_member)
#endif

#if !defined(container_of)
#define container_of(_ptr, _type, _member) ({ \
	const typeof( ((_type *)0)->_member ) *__mptr = (_ptr); \
	(_type *)( (char *)__mptr - offsetof(_type,_member) );})
#endif

struct list_entry {
	struct list_entry *prev;
	struct list_entry *next;
	bool in_use;
	mtx_t *list_mtx;
};

struct list {
	mtx_t mtx;
	struct list_entry head;
};

#define list_entry(_ptr, _type, _member, _list) \
	(&container_of(_ptr, _type, _member)->_member == &((_list)->head) \
	? NULL \
	: container_of(_ptr, _type, _member))

#define list_prev_entry(_list, _entry, _member) \
	list_entry(_entry->_member.prev, typeof(*_entry), _member, _list)

#define list_next_entry(_list, _entry, _member) \
	list_entry(_entry->_member.next, typeof(*_entry), _member, _list)

#define list_for_each(_list, _entry, _member) \
	for (_entry = list_entry((_list)->head.next, typeof(*_entry), _member, _list); \
		_entry; _entry = list_next_entry(_list, _entry, _member))

#define list_for_each_continue(_list, _entry, _member) \
	for (_entry = list_next_entry(_list, _entry, _member); _entry; _entry = list_next_entry(_list, _entry, _member))

#define list_for_each_safe(_list, _entry, _tmp, _member) \
	for (_entry = container_of((_list)->head.next, typeof(*_entry), _member), \
		_tmp = container_of(_entry->_member.next, typeof(*_entry), \
				_member); \
	     &_entry->_member != &(_list)->head; \
	     _entry = _tmp, \
	     _tmp = container_of(_tmp->_member.next, typeof(*_entry), _member))

#define DEFINE_LIST(_list) struct list _list = { \
	.head = { \
		.next = &_list.head, \
		.prev = &_list.head \
		.list_mtx = &_list.mtx \
	}, \
	.mtx = ???, \
}

#define STATIC_LIST(_list) static DEFINE_LIST(_list)

void list_init(struct list *list, const char *name);

void list_insert_before(struct list_entry *next, struct list_entry *le);
void list_insert_after(struct list_entry *prev, struct list_entry *le);

struct list_entry *list_get_first(struct list *list);
void list_remove(struct list_entry *le);

unsigned int list_item_count(const struct list *list);
bool list_is_empty(const struct list *list);

static inline void list_entry_init(struct list *list, struct list_entry *le)
{
	le->in_use = false;
	le->list_mtx = &list->mtx;
	//debug("%p %p %p %p\n", list, &list->mtx, le, le->list_mtx);
}

static inline void list_add_head(struct list *list, struct list_entry *le)
{
	list_entry_init(list, le);
	list_insert_after(&list->head, le);
}

void list_add_tail(struct list *list, struct list_entry *le);

#if defined(DEBUG_LIST)
# define list_lock(_m) do {_list_lock_debug(_m, __func__, __LINE__);} while(0)
# define list_unlock(_m) do {_list_unlock_debug(_m, __func__, __LINE__);} while(0)
#else
# define list_lock(_m) do {_list_lock(_m);} while(0)
# define list_unlock(_m) do {_list_unlock(_m);} while(0)
#endif

void _list_lock_debug(mtx_t *list_mtx, const char *func, int line);
void _list_unlock_debug(mtx_t *list_mtx, const char *func, int line);
void _list_lock(mtx_t *list_mtx);
void _list_unlock(mtx_t *list_mtx);

#endif /* _LIB_LIST_H */
