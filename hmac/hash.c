#if 0
#!/bin/sh

# hash example
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc $0 -lcrypto -o hash.out ; then
        echo "compile failed"
        exit 1
fi

sequence=1

hash_test () {
	local result expected=$1
	shift

	result=$(./hash.out "$@")
	if [ X"${result}" != X"${expected}" ] ; then
		echo "${sequence}: test failed"
		echo "${result}"
		return 1
	fi

	echo "${sequence}: test pass"
	let sequence++
}

hash_test \
	f3226f91f77a87d909b8920adc91f9a301a7316b \
	"--sha1" "gg" || exit $?

hash_test \
	f3226f91f77a87d909b8920adc91f9a301a7316b \
	"--sha1" "g" "g" || exit $?

hash_test \
	cbd3cfb9b9f51bbbfbf08759e243f5b3519cbf6ecc219ee95fe7c667e32c0a8d \
	"--sha256" "gg" || exit $?

hash_test \
	b114849cd171b9da51f770c7784deb9000ad2a2da3a7223183148f35a82458e46e8ef94887d8faf6c91807e0a3a7dff1 \
	"--sha384" "gg" || exit $?

hash_test \
	b7c3bd1e3976deb58236e6fb91da0cd5f4b0c2f6290cdc2b6f17c6da88d000420ec2d5d73b3e1e8ae14cafeabafe117a58060f427a66bdab1b97cf2d52aa0a94 \
	"--sha512" "gg" || exit $?

hash_test \
	73c18c59a39b18382081ec00bb456d43 \
	"--md5" "gg" || exit $?

exit 0
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>

static int xdigit(char ch)
{
	int xch = toupper(ch);

	return xch >= 'A' ? xch - 'A' + 10 : xch - '0';
}

static int xstring(const char *str, const char **endp, uint8_t *buf, size_t len)
{
	const char *p = str;
	int i = 0;

	while (*p != '\0' && i < (int)len) {
		if (!isxdigit(p[0])) { /* skip split symbol */
			++p;
			continue;
		} else if (!isxdigit(p[1])) {
			break;
		}

		uint8_t b = (xdigit(p[0]) << 4) | xdigit(p[1]);
		buf[i++] = b;
		p += 2;
	}

	if (endp)
		*endp = p;

	return i;
}

static size_t read_string(uint8_t *buf, size_t bufsz, const char *s)
{
	size_t len = 0;

	if (!strncmp(s, "0x", 2)) {
		const char *endp;

		len = xstring(s + 2, &endp, buf, bufsz);
		if (*endp != '\0') {
			fprintf(stderr, "Wrong hex string: %s\n", s);
			return 0;
		}
	} else {
		snprintf((char *)buf, bufsz, "%s", s);
		len = strlen((char *)buf);
	}

	return len;
}

enum hash_type {
	HASH_TYPE_SHA1,
	HASH_TYPE_SHA256,
	HASH_TYPE_SHA384,
	HASH_TYPE_SHA512,
	HASH_TYPE_MD5,

	HASH_TYPE_MAX,
};

struct hash {
	const char *name;
	const EVP_MD *(*get_evp_md)(void);
};

static const struct hash hash_algos[HASH_TYPE_MAX] = {
	[HASH_TYPE_SHA1] = {
		.name = "sha1",
		.get_evp_md = EVP_sha1,
	},

	[HASH_TYPE_SHA256] = {
		.name = "sha256",
		.get_evp_md = EVP_sha256,
	},

	[HASH_TYPE_SHA384] = {
		.name = "sha384",
		.get_evp_md = EVP_sha384,
	},

	[HASH_TYPE_SHA512] = {
		.name = "sha512",
		.get_evp_md = EVP_sha512,
	},

	[HASH_TYPE_MD5] = {
		.name = "md5",
		.get_evp_md = EVP_md5,
	}
};

static const struct hash *get_hash_algo(const char *name)
{
	for (enum hash_type t = 0; t < HASH_TYPE_MAX; t++) {
		const struct hash *h = &hash_algos[t];

		if (!strcmp(h->name, name))
			return h;
	}

	return NULL;
}

static unsigned hash_digest(const struct hash *h, const void *data, size_t sz,
			    uint8_t *digest, size_t digest_sz)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	uint8_t tmp[512 / 8];
	unsigned int len;

	EVP_DigestInit(ctx, h->get_evp_md());
	EVP_DigestUpdate(ctx, data, sz);
	EVP_DigestFinal(ctx, tmp, &len);
	EVP_MD_CTX_free(ctx);

	if (len > digest_sz)
		len = digest_sz;
	memcpy(digest, tmp, len);

	return len;
}

static struct option long_options[] = {
	/* name		has_arg,		*flag,	val */
	{ "help",	no_argument,		NULL,	'h'			},
	{ "sha1",	no_argument,		NULL,	HASH_TYPE_SHA1		},
	{ "sha256",	no_argument,		NULL,	HASH_TYPE_SHA256	},
	{ "sha384",	no_argument,		NULL,	HASH_TYPE_SHA384	},
	{ "sha512",	no_argument,		NULL,	HASH_TYPE_SHA512	},
	{ "md5",	no_argument,		NULL,	HASH_TYPE_MD5		},
	{ NULL,		0,			NULL,	0  			},
};

static void hash_usage(void)
{
	fprintf(stderr, "Usage: hash.out [OPTIONS] string1 string2...\n");
	fprintf(stderr, " -h --help:                   Show this informations\n");
	for (enum hash_type t = 0; t < HASH_TYPE_MAX; t++) {
		const struct hash *h = &hash_algos[t];

		fprintf(stderr, "   --%s:                   %s\n",
			h->name, h->name);
	}
	fprintf(stderr, "The input string can be hex number if starting with 0x\n");
}

int main(int argc, char **argv)
{
	enum hash_type type = HASH_TYPE_SHA1; /* default */
	uint8_t input[1024], md[64];
	size_t input_sz = 0;
	unsigned md_len;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;

		if (c >= 0 && c < HASH_TYPE_MAX) {
			type = c;
			continue;
		}

		switch (c) {
		case 'h':
			hash_usage();
			return 0;
		default:
			hash_usage();
			return -1;
		}
	}

	for (int i = optind; i < argc; i++) {
		size_t n;

		n = read_string(input + input_sz, sizeof(input) - input_sz,
				argv[i]);
		if (n == 0)
			return -1;

		input_sz += n;
	}

	md_len = hash_digest(&hash_algos[type], input, input_sz, md, sizeof(md));
	for (unsigned int i = 0; i < md_len; i++)
		printf("%02x", md[i]);
	printf("\n");

	return 0;
}
