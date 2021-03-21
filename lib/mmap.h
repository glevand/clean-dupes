/*
 *  Memory mapped file.
 */

#if !defined(_LIB_MAP_H)
#define _LIB_MAP_H

struct mapped_file_info {
	int fd;
	void *addr;
	size_t size;
};

void mapped_file_map(struct mapped_file_info *mfi, const char *file);
void mapped_file_unmap(struct mapped_file_info *mfi);

#endif /* _LIB_MAP_H */
