/*
 *  malloc wrappers.
 */

#if !defined(_LIB_MEM_H)
#define _LIB_MEM_H

#include <stdlib.h>

//#define DEBUG_MEM

void *mem_alloc(size_t size);
void *mem_alloc_zero(size_t size);
#if defined(DEBUG_MEM)
# define mem_free(_p) do {_mem_free_debug(_p, __func__, __LINE__);} while(0)
#else
# define mem_free(_p) do {_mem_free(_p);} while(0)
#endif
void _mem_free_debug(void *p, const char *func, int line);
void _mem_free(void *p);

char *mem_strdup(const char *str);
char *mem_strdupcat(const char *str1, const char *str2);

#endif /* _LIB_MEM_H */
