#if 0
#!/bin/sh

# Pseudo-Random Function example based on <ieee 802.11i-2004.pdf>
# PRF-n: pseudo random functioni producing n bits of output, defined in 8.5.1.1
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc -Wall -g -O0 $0 -lcrypto -o prf.out ; then
        echo "compile failed"
        exit 1
fi

sequence=1

prf_test () {
	local result expected=$1
	shift

	result=$(./prf.out "$@" | tr --delete '\n')
	if [ X"${result}" != X"${expected}" ] ; then
		echo "${sequence}: test failed"
		echo "${result}"
		return 1
	fi

	echo "${sequence}: test pass"
	let sequence++
}

# H.3.2 PRF test vextors
# Sequence 1
prf_test \
	"0xbcd4c650b30b9684951829e0d75f9d540xb862175ed9f00606e17d8da35402ffee0x75df78c3d31e0f889f012120c0862beb0x67753e7439ae242edb8373698356cf5a" \
	"--key" "0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b" \
	"--prefix" "prefix" \
	"--data" "Hi There" \
	|| exit $?

# Sequence 2
prf_test \
	"0x51f4de5b33f249adf81aeb713a3c20f40xfe631446fabdfa58244759ae58ef90090xa99abf4eac2ca5fa87e692c440eb40020x3e7babb206d61de7b92f41529092b8fc" \
	"--key" "Jefe" \
	"--prefix" "prefix" \
	"--data" "what do ya want for nothing?" \
	|| exit $?

# Sequence 3
prf_test \
	"0xe1ac546ec4cb636f9976487be5c86be10x7a0252ca5d8d8df12cfb0473525249ce0x9dd8d177ead710bc9b590547239107ae0xf7b4abd43d87f0a68f1cbd9e2b6f7607" \
	"--key" "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
	"--prefix" "prefix" \
	"--data" "0xdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd" \
	|| exit $?

exit 0
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

/* the out size in bytes */
#define PRF128_OUTSZ		(128 / 8)
#define PRF192_OUTSZ		(192 / 8)
#define PRF256_OUTSZ		(256 / 8)
#define PRF384_OUTSZ		(384 / 8)
#define PRF512_OUTSZ		(512 / 8)

static void mempush(uint8_t *buf, size_t bufsz, size_t *ret_idx,
		    const uint8_t *src, size_t len)
{
	size_t idx = *ret_idx, remain = bufsz - idx;

	if (remain < len)
		len = remain;

	memcpy(buf + idx, src, len);
	(*ret_idx) += len;
}

/* Based on H.3.1 PRF reference code
 *
 * PRF --- Length of output is in octets rather that bits
 *         since length is always a multiple of 8 output array is
 *         organized so first N octets startting from 0 containes PRF output
 *
 *         supported inputs are 16, 24, 32, 48, 64
 */
void ieee80211i_prf(const uint8_t *key, size_t key_len,
		    const uint8_t *prefix, size_t prefix_len,
		    const uint8_t *data, size_t data_len,
		    uint8_t *out, size_t len)
{
	uint8_t zero = 0, input[1024]; /* concatenated input */
	size_t currentindex = 0;
	size_t total_len = 0;

	mempush(input, sizeof(input), &total_len, prefix, prefix_len);
	mempush(input, sizeof(input), &total_len, &zero, sizeof(zero));

	mempush(input, sizeof(input), &total_len, data, data_len);
	mempush(input, sizeof(input), &total_len, &zero, sizeof(zero));

	for (int i = 0; i < (len + 19) / 20; i++) {
		uint8_t md[EVP_MAX_MD_SIZE];

		if (!HMAC(EVP_sha1(), key, key_len, input, total_len, md, NULL))
			ERR_clear_error();

		memcpy(&out[currentindex], md, 20);
		currentindex += 20; /* next concatenation location */

		input[total_len - 1]++; /* increment octet count */
	}
}

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

static struct option long_options[] = {
	/* name		has_arg,			*flag,		val */
	{ "help",	no_argument,			NULL,		'h' },
	{ "key",	required_argument,		NULL,		'k' },
	{ "prefix",	required_argument,		NULL,		'p' },
	{ "data",	required_argument,		NULL,		'd' },
	{ "bit",	required_argument,		NULL,		'b' },
	{ NULL,		0,				NULL,		 0  },
};

static void prf_usage(void)
{
	fprintf(stderr, "Usage: prf.out [OPTIONS]\n");
	fprintf(stderr, " -k --key key                 Input the key\n");
	fprintf(stderr, " -p --prefix prefix           Input the prefix\n");
	fprintf(stderr, " -d --data data               Input the data\n");
	fprintf(stderr, " -b --bit bit                 The output bits, can be 128, 192, 256, 385, 512(default)\n");
	fprintf(stderr, " -h --help:                   Show this informations\n");
	fprintf(stderr, "The input string can be hex number if starting with 0x\n");
}

int main(int argc, char *argv[])
{
	size_t key_len = 0, prefix_len = 0, data_len = 0;
	uint8_t key[64], prefix[64], data[512];
	uint8_t prf[PRF512_OUTSZ];
	int bits = 512;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "hk:p:d:b:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			prf_usage();
			return 0;
		case 'k':
			key_len = read_string(key, sizeof(key), optarg);
			if (!key_len)
				return -1;
			break;
		case 'p':
			prefix_len = read_string(prefix, sizeof(prefix), optarg);
			if (!prefix_len)
				return -1;
			break;
		case 'd':
			data_len = read_string(data, sizeof(data), optarg);
			if (!data_len)
				return -1;
			break;
		case 'b':
			bits = atoi(optarg);
			switch (bits) {
			case 128:
			case 192:
			case 256:
			case 384:
			case 512:
				break;
			default:
				prf_usage();
				return -1;
			}
			break;
		default:
			prf_usage();
			return -1;
		}
	}

	if (key_len == 0 || prefix_len == 0 || data_len == 0) {
		prf_usage();
		return -1;
	}

	ieee80211i_prf(key, key_len, prefix, prefix_len, data, data_len,
		       prf, bits / 8);

	for (int i = 0; i < bits / 8; i += 16) {
		int sz = bits / 8 - i;

		if (sz > 16)
			sz = 16;

		printf("0x");
		for (int j = 0; j < sz; j++)
			printf("%02x", prf[i + j]);
		printf("\n");
	}

	return 0;
}
