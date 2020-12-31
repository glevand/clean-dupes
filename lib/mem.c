#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "mem.h"

#if defined (DEBUG_MEM)
# define mem_debug log_raw
#else
# define mem_debug(_args...) while(0) {_debug(__func__, __LINE__, _args);}
#endif

static const unsigned int mem_magic = 0xa600d1U;

struct mem_header {
	unsigned int magic;
	size_t size;
	bool free_called;
};

void *mem_alloc(size_t size)
{
	void *p;
	struct mem_header *h;

	if (size == 0) {
		log("ERROR: Zero size alloc.\n");
		exit(EXIT_FAILURE);
	}

	h = malloc(sizeof(struct mem_header) + size);

	if (!h) {
		log("ERROR: calloc %lu failed: %s.\n", (unsigned long)size,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	h->magic = mem_magic;
	h->size = size;
	h->free_called = false;

	p = (void *)h + sizeof(struct mem_header);

	mem_debug("h=%p, p=%p\n", h, p);

	return p;
}

void *mem_alloc_zero(size_t size)
{
	void *p = mem_alloc(size);

	memset(p, 0, size);
	return p;
}

void _mem_free(void *p)
{
	if (!p) {
		log("ERROR: null free.\n");
		assert(0);
		exit(EXIT_FAILURE);
	}

	struct mem_header *h = p - sizeof(struct mem_header);

	mem_debug("h=%p, p=%p\n", h, p);

	if (h->magic != mem_magic) {
		log("ERROR: bad object.\n");
		assert(0);
		exit(EXIT_FAILURE);
	}

	if (h->free_called) {
		log("ERROR: double free.\n");
		assert(0);
		exit(EXIT_FAILURE);
	}

	h->free_called = true;
#if defined (DEBUG_MEM)
	memset(p, 0xcd, h->size);
#else
	free(h);
#endif
}

void __attribute__ ((unused)) _mem_free_debug(void *p, const char *func,
	int line)
{
	mem_debug("=>%s:%d: %p\n", func, line, p);
	_mem_free(p);
}

char *mem_strdup(const char *str1)
{
	size_t len1;
	char *new;

	len1 = strlen(str1);
	new = mem_alloc(len1 + 1);
	memcpy(new, str1, len1 + 1);

	return new;
}

char *mem_strdupcat(const char *str1, const char *str2)
{
	size_t len1;
	size_t len2;
	char *new;

	len1 = strlen(str1);
	len2 = strlen(str2);
	new = mem_alloc(len1 + len2 + 1);
	memcpy(new, str1, len1);
	memcpy(new + len1, str2, len2 + 1);

	return new;
}
