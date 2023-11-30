#if 0
#!/bin/sh

# AES example
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc $0 -lcrypto -o aes.out ; then
        echo "compile failed"
        exit 1
fi

sequence=1

aes_test () {
	local result expected=$1
	shift

	result=$(./aes.out "$@")
	if [ X"${result}" != X"${expected}" ] ; then
		echo "${sequence}: test failed"
		echo "${result}"
		return 1
	fi

	echo "${sequence}: test pass"
	let sequence++
}

# the test database is copied from wsbrd.
key=0x0349144194681655ec5ab1d8f8451109

db1raw=dd16000fac010000461d435d6fa20994287b108632fcf6ffdd08000fac0700278c6cdd050c5a9e0201dd000000000000
db1wrp=108194534fb1f64dd062e19da756402a401ddce6c8ff8845c538fc5dc0c1089c83566b1b9cb410b580b9f789f5e16b8d21c3b1f7bd7415b6

aes_test \
	${db1wrp} \
	"--wrap" "--key" "${key}" "0x${db1raw}" || exit $?

aes_test \
	${db1raw} \
	"--unwrap" "--key" "${key}" "0x${db1wrp}" || exit $?

exit 0
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <openssl/aes.h>
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

/*
 * @out: Plaintext without IV.
 *       Minimal buffer length = (inlen - 8) bytes.
 * Return 0 if failed or the out length.
 */
static int aes_key_unwrap(const uint8_t *ukey, size_t ukey_sz,
			  const uint8_t *in, size_t inlen,
			  uint8_t *out)
{
	AES_KEY key;

	AES_set_decrypt_key(ukey, ukey_sz * 8 /* to bits */, &key);

	return AES_unwrap_key(&key, NULL, out, in, inlen);
}

/*
 * @out: Ciphertext. Minimal buffer length = (inlen + 8) bytes.
 * Return 0 if failed or the out length
 */
static int aes_key_wrap(const uint8_t *ukey, size_t ukey_sz,
			const uint8_t *in, size_t inlen,
			uint8_t *out)
{
	AES_KEY key;

	AES_set_encrypt_key(ukey, ukey_sz * 8, &key);

	return AES_wrap_key(&key, NULL, out, in, inlen);
}

static struct option long_options[] = {
	/* name		has_arg,		*flag,	val */
	{ "help",	no_argument,		NULL,	'h'		},
	{ "key",	required_argument,	NULL,	'K'		},
	{ "wrap",	no_argument,		NULL,	'W'		},
	{ "unwrap",	no_argument,		NULL,	'U'		},
	{ NULL,		0,			NULL,	0  		},
};

static void aes_usage(void)
{
	fprintf(stderr, "Usage: aes.out [OPTIONS] string1\n");
	fprintf(stderr, " -h --help:                   Show this informations\n");
	fprintf(stderr, "    --key key:                Key string\n");
	fprintf(stderr, "    --wrap:                   AES key wrap\n");
	fprintf(stderr, "    --unwrap:                 AES key unwrap\n");
	fprintf(stderr, "The input string can be hex number if starting with 0x\n");
}

int main(int argc, char **argv)
{
	uint8_t input[1024], out[sizeof(input) + 8], key[64];
	size_t input_sz = 0, key_sz = 0;
	int wrap = 1;
	int ret = -1;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			aes_usage();
			return 0;
		case 'W': /* --wrap */
		case 'U': /* --unwrap */
			wrap = c == 'W';
			break;
		case 'K': /* --key */
			key_sz = read_string(key, sizeof(key), optarg);
			if (!key_sz)
				return -1;
			break;
		default:
			aes_usage();
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

	if (!wrap)
		ret = aes_key_unwrap(key, key_sz, input, input_sz, out);
	else
		ret = aes_key_wrap(key, key_sz, input, input_sz, out);

	if (ret <= 0) {
		fprintf(stderr, "failed\n");
		return -1;
	}

	for (int i = 0; i < ret; i++)
		printf("%02x", out[i]);
	printf("\n");

	return 0;
}
