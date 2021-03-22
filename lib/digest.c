/*
 * Message digests.
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

//#define DEBUG 1

#include <assert.h>
#include <errno.h>

#if defined(HAVE_MURMURHASH_H)
# include <murmurhash.h>
#endif

#include <openssl/evp.h>

#include "digest.h"
#include "log.h"
#include "mmap.h"

void digest_init_type(struct digest *digest, enum digest_type type)
{
	if (type != digest_type_md5sum && type != digest_type_mmhash) {
		log("ERROR: Bad digest type: %d\n", type);
		assert(0);
		exit(EXIT_FAILURE);
	}

	digest->type = type;
	digest->data[0] = 0;
	//debug("type = %d\n", digest->type);
}

static void digest_md5sum_file(struct digest *digest,
	struct mapped_file_info *mfi)
{
	unsigned int digest_len;
	EVP_MD_CTX *ctx;

	debug("\n");

	assert(digest->type == digest_type_md5sum);

	ctx = EVP_MD_CTX_create();

	EVP_DigestInit(ctx, EVP_md5());
	EVP_DigestUpdate(ctx, mfi->addr, mfi->size);
	EVP_DigestFinal(ctx, (void *)digest->data, &digest_len);

	EVP_MD_CTX_destroy(ctx);

	if (digest_len != sizeof(digest->data)) {
		log("ERROR: Bad digest length: %u != %lu\n", digest_len,
			sizeof(digest->data));
		assert(0);
		exit(EXIT_FAILURE);
	}
}

int digest_hash_file(struct digest *digest, const char *file)
{
	struct mapped_file_info mfi;

	mapped_file_map(&mfi, file);

	switch (digest->type) {
	case digest_type_md5sum:
		debug("'%s' digest_type_md5sum\n", file);
		digest_md5sum_file(digest, &mfi);
		break;

#if defined(HAVE_MURMURHASH_H)
	case digest_type_mmhash:
	{
		static const uint32_t mmhash_seed = 0;

		//debug("'%s' digest_type_mmhash\n", file);
		lmmh_x64_128(mfi.addr, mfi.size, mmhash_seed, (uint64_t *)digest->data);
		break;
	}
#endif

	default:
		log("Internal error: %d\n", digest->type);
		assert(0);
		exit(EXIT_FAILURE);
	}

	mapped_file_unmap(&mfi);

	if (0) {
		struct digest_str s;
		digest_sprint(digest, &s);
		debug("done: '%s' = %s\n", file, s.str);
	}

	return 0;
}

int digest_sprint(const struct digest *digest, struct digest_str *digest_str)
{
	return sprintf(digest_str->str, "%" PRIx64 "%" PRIx64 "",
		digest->data[0], digest->data[1]);
}

int digest_fprint(const struct digest *digest, FILE *fp)
{
	return fprintf(fp, "%" PRIx64 "%" PRIx64 "", digest->data[0],
		digest->data[1]);
}
