#if !defined(_LIB_WORK_QUEUE)
#define _LIB_WORK_QUEUE

#include <semaphore.h>

#include "thread-pool.h"
#include "list.h"

struct work_item;
struct work_queue;

typedef int (*work_item_cb)(struct work_item *wi);

struct work_item {
	unsigned int id;
	work_item_cb cb;
	void* cb_data;
	void* result;
	struct list_entry list_entry;
	struct list *list;
	struct work_queue *wq;
};

struct work_queue {
	sem_t work_ready;
	bool exit;
	struct list ready_list;
	struct list done_list;
	struct thread_pool *thread_pool;
};

struct work_queue *work_queue_alloc(unsigned int thread_count);
void work_queue_init(struct work_queue *wq, unsigned int thread_count);
void work_queue_exit(struct work_queue *wq);
void work_queue_delete(struct work_queue *wq);

void work_queue_add_item(struct work_queue *wq, struct work_item *wi);
struct work_item *work_queue_get_item(struct work_queue *wq);
void work_queue_finish_item(struct work_item *wi);
void work_queue_empty_ready_list(struct work_queue *wq);

void __attribute__ ((unused)) work_queue_test(void);

#endif /* _LIB_WORK_QUEUE */
