#if !defined(_LIB_THREAD_POOL)
#define _LIB_THREAD_POOL

#include <pthread.h>
#include <semaphore.h>
#include <threads.h>

#include "list.h"

struct thread_pool;

typedef void (*thread_pool_run_fn)(unsigned int id, void *run_data);

struct thread_pool_entry {
	pthread_t t_id;
	unsigned int id;
	thread_pool_run_fn run_fn;
	void *run_data;
	struct thread_pool *tp;
};

struct thread_pool {
	unsigned int count;
	bool exit;
	struct thread_pool_entry pool[];
};

struct thread_pool *thread_pool_init(unsigned int count,
	thread_pool_run_fn run_fn, void *run_data);

void thread_pool_exit(struct thread_pool *tp);
void thread_pool_delete(struct thread_pool *tp);

void __attribute__ ((unused)) thread_pool_test(void);

#endif /* _LIB_THREAD_POOL */
