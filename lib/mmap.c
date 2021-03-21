/*
 *  Memory mapped file.
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

//#define DEBUG 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "log.h"
#include "mmap.h"

void mapped_file_map(struct mapped_file_info *mfi, const char *file)
{
	struct stat stat;
	int result;

	//debug("start:  '%s'\n", file);

	memset(mfi, 0, sizeof(*mfi));

	mfi->fd = open(file, O_RDONLY);

	if (mfi->fd < 0) {
		log("ERROR: open '%s' failed: %s\n", file, strerror(errno));
		assert(0);
		exit(EXIT_FAILURE);
	}

	result = fstat(mfi->fd, &stat);

	if (result < 0) {
		log("ERROR: fstat '%s' failed: %s\n", file, strerror(errno));
		assert(0);
		exit(EXIT_FAILURE);
	}

	mfi->size = stat.st_size;

	mfi->addr = mmap(NULL, mfi->size, PROT_READ, MAP_SHARED, mfi->fd, 0);

	if (mfi->addr == MAP_FAILED) {
		log("ERROR: mmap '%s' failed: %s\n", file, strerror(errno));
		assert(0);
		exit(EXIT_FAILURE);
	}
	debug("mapped: '%s', %lu bytes\n", file, (unsigned long)mfi->size);
}

void mapped_file_unmap(struct mapped_file_info *mfi)
{
	//debug("\n");
	munmap(mfi->addr, mfi->size);
	close(mfi->fd);
	memset(mfi, 0xbc, sizeof(*mfi));
}
