/*
 *  cryptographic digests.
 */

#if !defined(_LIB_DIGEST_H)
#define _LIB_DIGEST_H

#include <stdbool.h>
#include <stdio.h>

#define MD5SUM_DIGEST_LEN (16)
#define MD5SUM_DIGEST_STR_LEN (16 * 2 + 1)
static const unsigned int md5sum_digest_len = MD5SUM_DIGEST_LEN;
static const unsigned int md5sum_str_len = MD5SUM_DIGEST_STR_LEN;

struct md5sum {
	unsigned char digest[MD5SUM_DIGEST_LEN];
};

struct md5sum_str {
	char str[MD5SUM_DIGEST_STR_LEN];
};

int md5sum_file(struct md5sum *md5sum, const char *file);
void md5sum_sprint(const struct md5sum *md5sum, struct md5sum_str *md5sum_str);
void md5sum_fprint(const struct md5sum *md5sum, FILE *fp);
bool md5sum_compare(const struct md5sum *md5sum1, const struct md5sum *md5sum2);

static inline void md5sum_init(struct md5sum *md5sum)
{
	md5sum->digest[0] = 0;
}

static inline bool md5sum_empty(const struct md5sum *md5sum)
{
	return !md5sum->digest[0];
}

#endif /* _LIB_DIGEST_H */
