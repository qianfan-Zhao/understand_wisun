#if 0
#!/bin/sh

# HMAC(Hash-Based Message Authentication Codes) example
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc $0 -lcrypto -o hmacsha1.out ; then
        echo "compile failed"
        exit 1
fi

sequence=1

hmacsha1_test () {
	local result expected=$1
	shift

	result=$(./hmacsha1.out "$@")
	if [ X"${result}" != X"${expected}" ] ; then
		echo "${sequence}: test failed"
		echo "${result}"
		return 1
	fi

	echo "${sequence}: test pass"
	let sequence++
}

hmacsha1_test \
	f919674aea01398d54ef3e6f0df419ddb9482b31 \
	0x11223344 \
	"gg" || exit $?

hmacsha1_test \
	f919674aea01398d54ef3e6f0df419ddb9482b31 \
	0x11223344 \
	"g" "g" || exit $?

exit 0
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
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

int main(int argc, char **argv)
{
	const char *endp, *key_str = argv[1];
	uint8_t input[1024], key[64], md[EVP_MAX_MD_SIZE];
	size_t input_sz = 0;
	unsigned int md_len;
	int key_len;

	if (argc < 3) {
		fprintf(stderr, "Usage: hmacsha1.out key datastring1, datastring2...\n");
		fprintf(stderr, "  key should be hex string\n");
		fprintf(stderr, "  if the datastring is started with 0x, we will translate it to hex number\n");
		return -1;
	}

	key_len = read_string(key, sizeof(key), key_str);
	if (!key_len)
		return -1;

	for (int i = 2; i < argc; i++) {
		size_t n;

		n = read_string(input + input_sz, sizeof(input) - input_sz,
				argv[i]);
		if (n == 0)
			return -1;

		input_sz += n;
	}

	if (!HMAC(EVP_sha1(), key, key_len, input, input_sz, md, &md_len))
		ERR_clear_error();

	for (unsigned int i = 0; i < md_len; i++)
		printf("%02x", md[i]);
	printf("\n");

	return 0;
}
