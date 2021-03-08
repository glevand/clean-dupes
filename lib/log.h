#if !defined(_LIB_LOG_H)
#define _LIB_LOG_H

#include <stdbool.h>
#include <stdio.h>

void set_verbosity(unsigned int v);
unsigned int get_verbosity(void);

void __attribute__ ((unused, format (printf, 3, 4))) _log(const char *func, int line,
	const char *fmt, ...);
void __attribute__ ((unused, format (printf, 1, 2))) log_raw(const char *fmt, ...);
#define log(_args...) do {_log(__func__, __LINE__, _args);} while(0)

void set_log_path(const char *log_path);
void set_log_fp(FILE *new_log);
void log_flush(void);

void __attribute__ ((unused, format (printf, 3, 4))) _debug(const char *func, int line,
	const char *fmt, ...);
void __attribute__ ((unused, format (printf, 1, 2))) debug_raw(const char *fmt, ...);
#define debug(_args...) do {_debug(__func__, __LINE__, _args);} while(0)

void set_debug_on(bool b);
bool get_debug_on(void);

void __attribute__ ((format (printf, 4, 5))) _on_error(const char *file,
	const char *func, int line, const char *fmt, ...);
#define on_error(_args...) do {_on_error(__FILE__, __func__, __LINE__, _args);} while(0)

#endif /* _LIB_LOG_H */
