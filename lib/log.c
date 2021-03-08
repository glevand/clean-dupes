#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "log.h"
#include "util.h"

#if defined(DEBUG)
static unsigned int verbosity = 10;
static bool debug_on = true;
#else
static unsigned int verbosity = 0;
static bool debug_on = false;
#endif

void set_verbosity(unsigned int v)
{
	verbosity = v;
}

unsigned int get_verbosity(void)
{
	return verbosity;
}

void set_debug_on(bool b)
{
	debug_on = b;
}

bool get_debug_on(void)
{
	return debug_on;
}

static FILE *log;

void log_flush(void)
{
	fflush(stderr);
	fflush(log);
}

void set_log_path(const char *log_path)
{
	assert(log_path);

	if (log && log != stderr) {
		fclose(log);
	}

	log = fopen(log_path, "a");

	if (!log) {
		fprintf(stderr, "ERROR: fopen log '%s' failed: %s\n", log_path, strerror(errno));
		exit(EXIT_FAILURE);
	}
	log_raw("-------| log open |-------\n");
}

void set_log_fp(FILE *new_log)
{
	assert(new_log);

	if (log && log != stderr) {
		fclose(log);
	}
	log = new_log;
}

static void __attribute__ ((unused)) _vlog(const char *func, int line,
	const char *fmt, va_list ap)
{
	static mtx_t lock;
	static int init_done;
	int result;
	
	if (!init_done) {
		init_done = 1;
		result = mtx_init(&lock, mtx_plain);

		if (result) {
			fprintf(stderr, "%s: error: mxt_init: %d\n", __func__, result);
			exit(EXIT_FAILURE);
		}
	}
		
	if (!log)
		log = stderr;

	mtx_lock(&lock);
	if (func)
		fprintf(log, "%s:%d: ", func, line);

	vfprintf(log, fmt, ap);
	fflush(log);
	mtx_unlock(&lock);
}

void  __attribute__ ((unused)) _log(const char *func, int line,
	const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vlog(func, line, fmt, ap);
	va_end(ap);
}

void  __attribute__ ((unused)) log_raw(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vlog(NULL, 0, fmt, ap);
	va_end(ap);
}

void  __attribute__ ((unused)) _debug(const char *func, int line,
	const char *fmt, ...)
{
	va_list ap;

	if (!debug_on)
		return;

	va_start(ap, fmt);
	_vlog(func, line, fmt, ap);
	va_end(ap);
}

void  __attribute__ ((unused)) debug_raw(const char *fmt, ...)
{
	va_list ap;

	if (!debug_on)
		return;

	va_start(ap, fmt);
	_vlog(NULL, 0, fmt, ap);
	va_end(ap);
}

void _on_error(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	fprintf(stderr, "%s:%s:%d: ERROR: ", file, func, line);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);

	va_end(ap);
	assert(0);
	exit(EXIT_FAILURE);
}

