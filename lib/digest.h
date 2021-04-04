/*
 * Message digests.
 */

#if !defined(_LIB_DIGEST_H)
#define _LIB_DIGEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum digest_type {
	digest_type_md5sum = 111,
	digest_type_mmhash,
};

struct digest {
	uint64_t data[2];
	enum digest_type type;
};

struct digest_str {
	char str[32 + 1];
};

void digest_init_type(struct digest *digest, enum digest_type type);
int digest_hash_file(struct digest *digest, const char *file);
int digest_sprint(const struct digest *digest, struct digest_str *digest_str);
int digest_fprint(const struct digest *digest, FILE *fp);

static inline void digest_init(struct digest *digest)
{
#if defined(HAVE_MURMURHASH_H)
	digest->type = digest_type_mmhash;
#else
	digest->type = digest_type_md5sum;
#endif
	digest->data[0] = digest->data[1] = 0;
}

static inline bool digest_is_empty(const struct digest *digest)
{
	return !digest->data[0] && !digest->data[1];
}

static inline bool digest_compare(const struct digest *digest1,
	const struct digest *digest2)
{
	return (digest1->data[0] == digest2->data[0]
		&& digest1->data[1] == digest2->data[1]);
}

static inline void digest_print(const struct digest *digest)
{
	digest_fprint(digest, stdout);
}

#endif /* _LIB_DIGEST_H */
