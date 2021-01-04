/*
 *  thread pool.
 */

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "thread-pool.h"
#include "log.h"
#include "mem.h"

//#define DEBUG_TP

#if defined(DEBUG_TP)
# define tp_debug(_args...) do {_debug(__func__, __LINE__, _args);} while(0)
#else
# define tp_debug(_args...) while(0) {_debug(__func__, __LINE__, _args);}
#endif

static void *thread_pool_start_fn(void *arg)
{
	struct thread_pool_entry *tpe = arg;

	assert(tpe);
	assert(tpe->run_fn);
	assert(tpe->run_data);
	assert(tpe->tp);
	assert(!tpe->tp->exit);

	while (!tpe->tp->exit) {
		tp_debug("th%u: calling run_fn\n", tpe->id);
		tpe->run_fn(tpe->id, tpe->run_data);
		tp_debug("th%u: run_fn returned\n", tpe->id);
	};

	tp_debug("th%u: start_fn exit\n", tpe->id);
	return NULL;
}

struct thread_pool *thread_pool_init(unsigned int count,
	thread_pool_run_fn run_fn, void *run_data)
{
	struct thread_pool *tp;
	unsigned int i;

	tp_debug("creating %u threads\n", count);

	tp = mem_alloc_zero(sizeof(*tp) + count * sizeof(tp->pool[0]));
	tp->count = count;
	tp->exit = false;

	for (i = 0; i < tp->count; i++) {
		struct thread_pool_entry *tpe = &tp->pool[i];
		int result;

		tpe->id = i;
		tpe->run_fn = run_fn;
		tpe->run_data = run_data;
		tpe->tp = tp;

		__sync_synchronize();

		result = pthread_create(&tpe->t_id, NULL, thread_pool_start_fn,
			tpe);

		tp_debug("th%u: created\n", tpe->id);

		if (result) {
			on_error("pthread_create.\n");
		}
	}

	return tp;
}

void thread_pool_exit(struct thread_pool *tp)
{
	tp->exit = true;
	__sync_synchronize();
}

void thread_pool_delete(struct thread_pool *tp)
{
	unsigned int i;

	tp_debug(">\n");
	
	thread_pool_exit(tp);

	for (i = 0; i < tp->count; i++) {
		struct thread_pool_entry *tpe = &tp->pool[i];
		void *thread_result;
		int result;

		result = pthread_join(tpe->t_id, &thread_result);

		tp_debug("joined th%u: %p = %lu\n", tpe->id,
			thread_result, (unsigned long)thread_result);

		if (result) {
			on_error("pthread_join.\n");
		}
	}

	mem_free(tp);
	tp_debug("<\n");
}

#define test_threads 2
#define test_sleep 5

__attribute__ ((unused)) struct thread_pool_test_data {
	unsigned int counter[test_threads];
};

static void __attribute__ ((unused)) thread_pool_test_run_fn(unsigned int id,
	void *run_data)
{
	struct thread_pool_test_data *data = run_data;

	data->counter[id]++;

	tp_debug("th%u.%u sleeping...\n", id, data->counter[id]);
	sleep(2);
	tp_debug("th%u.%u woke\n", id, data->counter[id]);
}

void __attribute__ ((unused)) thread_pool_test(void)
{
	struct thread_pool_test_data run_data = {0};
	struct thread_pool *tp;

	tp_debug("testing: threads = %u, sleep = %u\n", test_threads, test_sleep);

	tp = thread_pool_init(test_threads,
		thread_pool_test_run_fn, &run_data);

	tp_debug("sleeping %u...\n", test_sleep);
	sleep(test_sleep);
	tp_debug("woke\n");

	thread_pool_delete(tp);

	tp_debug("<\n");
}
