#if 0
#!/bin/sh

# HMAC(Hash-Based Message Authentication Codes) example
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc $0 -lcrypto -o hmacsha1.out ; then
        echo "compile failed"
        exit 1
fi
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

int main(int argc, char **argv)
{
	const char *endp, *key_str = argv[1], *msg_str = argv[2];
	uint8_t key[64], md[EVP_MAX_MD_SIZE];
	unsigned int md_len;
	int key_len;

	if (argc != 3) {
		fprintf(stderr, "Usage: hmacsha1.out key string\n");
		return -1;
	}

	key_len = xstring(key_str, &endp, key, sizeof(key));
	if (*endp != '\0') {
		fprintf(stderr, "Error: Invalid hex key string\n");
		return -1;
	}

	if (!HMAC(EVP_sha1(), key, key_len, (const unsigned char *)msg_str,
			strlen(msg_str), md, &md_len))
		ERR_clear_error();

	for (unsigned int i = 0; i < md_len; i++)
		printf("%02x", md[i]);
	printf("\n");

	return 0;
}
