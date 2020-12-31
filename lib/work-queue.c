#include <assert.h>
#include <unistd.h>

#include "work-queue.h"
#include "log.h"
#include "mem.h"

//#define DEBUG_WQ

#if defined(DEBUG_WQ)
# define wq_debug(_args...) do {_debug(__func__, __LINE__, _args);} while(0)
#else
# define wq_debug(_args...) while(0) {_debug(__func__, __LINE__, _args);}
#endif

static void work_queue_run(__attribute__ ((unused)) unsigned int id,
	struct work_queue *wq)
{
	struct work_item *wi;

	wq_debug("th%u: wait work\n", id);

	wi = work_queue_get_item(wq);

	if (!wi) {
		wq_debug("th%u: work_queue_get_item returned NULL\n", id);
		return;
	}

	wq_debug("th%u: got work wi%u\n", id, wi->id);

	assert(wi->cb);

	wi->cb(wi); // cb takes ownership of wi.
}

void work_queue_init(struct work_queue *wq, unsigned int thread_count)
{
	int result;

	result = sem_init(&wq->work_ready, 0, 0);

	if (result) {
		on_error("sem_init");
	}

	wq->thread_pool = thread_pool_init(thread_count,
		(thread_pool_run_fn)work_queue_run, wq);
	
	list_init(&wq->ready_list, "work queue ready_list");
	list_init(&wq->done_list, "work queue done_list");
}

struct work_queue *work_queue_alloc(unsigned int thread_count)
{
	struct work_queue *wq;

	wq = mem_alloc_zero(sizeof(*wq));
	work_queue_init(wq, thread_count);

	return wq;
}

void work_queue_exit(struct work_queue *wq)
{
	unsigned int i;
	struct work_item *wi;

	list_for_each(&wq->ready_list, wi, list_entry) {
		wq_debug("wi = %u\n", wi->id);
	}

	thread_pool_exit(wq->thread_pool);

	wq->exit = 1;
	__sync_synchronize();

	for (i = 0; i < wq->thread_pool->count; i++) {
		sem_post(&wq->work_ready);
	}
}

void work_queue_delete(struct work_queue *wq)
{
	wq_debug(">\n");

	work_queue_exit(wq);
	thread_pool_delete(wq->thread_pool);
	mem_free(wq);

	wq_debug("<\n");
}

void work_queue_add_item(struct work_queue *wq, struct work_item *wi)
{
	wi->wq = wq;

	list_entry_init(&wq->ready_list, &wi->list_entry);
	list_add_tail(&wq->ready_list, &wi->list_entry);

	__sync_synchronize();

	wq_debug("wi%u added\n", wi->id);
	sem_post(&wq->work_ready);
}

struct work_item *work_queue_get_item(struct work_queue *wq)
{
	struct list_entry *le;
	struct work_item *wi;

	sem_wait(&wq->work_ready);
	
	if (wq->exit) {
		wq_debug("got wq exit\n");
		return NULL;
	}

	le = list_get_first(&wq->ready_list);

	if (!le) {
		on_error("ready_list empty");
	}
	
	wi = list_entry(le, struct work_item, list_entry, &wq->ready_list);

	return wi;
}

void work_queue_finish_item(struct work_item *wi)
{
	struct work_queue *wq = wi->wq;

	assert(wi->list_entry.in_use);

	list_remove(&wi->list_entry);

	list_add_tail(&wq->done_list, &wi->list_entry);
	__sync_synchronize();
}

void work_queue_empty_ready_list(struct work_queue *wq)
{
	struct work_item *wi_safe;
	struct work_item *wi;

	list_for_each_safe(&wq->ready_list, wi, wi_safe, list_entry) {
		list_remove(&wi->list_entry);
		mem_free(wi);
	}
}

#define test_threads 9
#define test_items 28

struct work_queue_test_cb_data {
	unsigned int data;
};

static int work_queue_test_cb(struct work_item *wi)
{
	struct work_queue_test_cb_data *cbd = wi->cb_data;

	wq_debug("wi%u: start, data = %d\n", wi->id, cbd->data++);

	sleep(2);

	wq_debug("wi%u: woke, data = %d\n", wi->id, cbd->data);

	wi->result = &wi->id;

	work_queue_finish_item(wi);

	return 0;
}

void __attribute__ ((unused)) work_queue_test(void)
{
	struct work_queue_test_cb_data *wi_cbd;
	struct work_queue *wq;
	struct work_item *items[test_items];
	unsigned int i;

	wq_debug(">\n");

	wq = work_queue_alloc(test_threads);

	wq_debug("creating %u work items\n", test_items);

	for (i = 0; i < test_items; i++) {
		struct work_item *wi;
		
		wi = mem_alloc_zero(sizeof(*wi) + sizeof(*wi_cbd));
		wi->id = i;
		wi->cb = work_queue_test_cb;

		wi_cbd = (void*)(wi + 1);
		wi_cbd->data = 10 * wi->id;
		wi->cb_data = wi_cbd;

		work_queue_add_item(wq, wi);

		items[i] = wi;
	}

	i = 1;
	while (!list_is_empty(&wq->ready_list)) {
		wq_debug("waiting for work to finish: %u\n", i++);
		sleep(1);
	}

	wq_debug("work done\n");

	work_queue_delete(wq);

	for (i = 0; i < test_items; i++) {
		mem_free(items[i]);
	}

	wq_debug("<\n");
}

