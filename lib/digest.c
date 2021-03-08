/*
 *  cryptographic digests.
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

//#define DEBUG 1

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>

#include "log.h"
#include "digest.h"

int md5sum_file(struct md5sum *md5sum, const char *file)
{
	unsigned int digest_len;
	EVP_MD_CTX *ctx;
	FILE *fp;

	//debug("'%s' start\n", file);

	ctx = EVP_MD_CTX_create();
	EVP_DigestInit(ctx, EVP_md5());

	fp = fopen(file, "r");

	if (!fp) {
		log("ERROR: fopen failed: %s\n", strerror(errno));
		assert(0);
		exit(EXIT_FAILURE);
	}

	while (1) {
		unsigned char buf[16 * 1024];
		size_t bytes;

		bytes = fread(buf, 1, sizeof(buf), fp);

		if (ferror(fp)) {
			log("ERROR: fread failed: %s\n", strerror(errno));
			fclose(fp);
			assert(0);
			exit(EXIT_FAILURE);
		}

		//debug("'%s' update: %u\n", file, (unsigned)bytes);
		EVP_DigestUpdate(ctx, buf, bytes);

		if (feof(fp)) {
			//debug("EOF\n");
			break;
		}

	}
	//debug("'%s' finish\n", file);

	fclose(fp);

	EVP_DigestFinal(ctx, md5sum->digest, &digest_len);
	EVP_MD_CTX_destroy(ctx);

	if (digest_len != md5sum_digest_len) {
		log("ERROR: Bad digest length: %u != %u\n", digest_len, md5sum_digest_len);
		assert(0);
		exit(EXIT_FAILURE);
	}

	if (0) {
		struct md5sum_str s;
		md5sum_sprint(md5sum, &s);
		debug("done: %s => %s\n", file, s.str);
	}
	return 0;
}

void md5sum_sprint(const struct md5sum *md5sum, struct md5sum_str *md5sum_str)
{
	unsigned int i;
	char *p;

	for (i = 0, p = md5sum_str->str; i < md5sum_digest_len; i++) {
		p += sprintf(p, "%02x", md5sum->digest[i]);
	}
}

void md5sum_fprint(const struct md5sum *md5sum, FILE *fp)
{
	unsigned int i;

	for (i = 0; i < md5sum_digest_len; i++) {
		fprintf(fp, "%02x", md5sum->digest[i]);
	}
}

bool md5sum_compare(const struct md5sum *md5sum1, const struct md5sum *md5sum2)
{
	return (md5sum1->digest[0] == md5sum2->digest[0]
		&& md5sum1->digest[1] == md5sum2->digest[1]
		&& md5sum1->digest[2] == md5sum2->digest[2]
		&& md5sum1->digest[3] == md5sum2->digest[3]
		&& md5sum1->digest[4] == md5sum2->digest[4]
		&& md5sum1->digest[5] == md5sum2->digest[5]
		&& md5sum1->digest[6] == md5sum2->digest[6]
		&& md5sum1->digest[7] == md5sum2->digest[7]
		&& md5sum1->digest[8] == md5sum2->digest[8]
		&& md5sum1->digest[9] == md5sum2->digest[9]
		&& md5sum1->digest[10] == md5sum2->digest[10]
		&& md5sum1->digest[11] == md5sum2->digest[11]
		&& md5sum1->digest[12] == md5sum2->digest[12]
		&& md5sum1->digest[13] == md5sum2->digest[13]
		&& md5sum1->digest[14] == md5sum2->digest[14]
		&& md5sum1->digest[15] == md5sum2->digest[15]);
}
