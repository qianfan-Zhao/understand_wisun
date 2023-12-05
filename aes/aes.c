#if 0
#!/bin/sh

# AES example
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc -Wall $0 -g -lcrypto -o aes.out ; then
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
		echo "${expected}"
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

# Appendix C - Example Vectors of AES standard.
#
# C.1 AES-128(Nk = 4, Nr = 10)
aes_test \
	"69c4e0d86a7b0430d8cdb78070b4c55a" \
	"--cipher" "--key" "0x000102030405060708090a0b0c0d0e0f" \
	"0x00112233445566778899aabbccddeeff" \
	|| exit $?
aes_test \
	"00112233445566778899aabbccddeeff" \
	"--invcipher" "--key" "0x000102030405060708090a0b0c0d0e0f" \
	"0x69c4e0d86a7b0430d8cdb78070b4c55a" \
	|| exit $?

# C.2 AES-192(Nk = 6, Nr = 12)
aes_test \
	"dda97ca4864cdfe06eaf70a0ec0d7191" \
	"--cipher" "--key" "0x000102030405060708090a0b0c0d0e0f1011121314151617" \
	"0x00112233445566778899aabbccddeeff" \
	|| exit $?
aes_test \
	"00112233445566778899aabbccddeeff" \
	"--invcipher" "--key" "0x000102030405060708090a0b0c0d0e0f1011121314151617" \
	"0xdda97ca4864cdfe06eaf70a0ec0d7191" \
	|| exit $?

# ECB test
aes_test \
	"69c4e0d86a7b0430d8cdb78070b4c55a69c4e0d86a7b0430d8cdb78070b4c55a" \
	"--ecb" "--key" "0x000102030405060708090a0b0c0d0e0f" \
	"0x00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff" \
	|| exit $?
aes_test \
	"00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff" \
	"--invecb" "--key" "0x000102030405060708090a0b0c0d0e0f" \
	"0x69c4e0d86a7b0430d8cdb78070b4c55a69c4e0d86a7b0430d8cdb78070b4c55a" \
	|| exit $?

# Recommendation for Block Cipher Modes of Operation
# nistspecialpublication800-38a.pdf

# F.2 CBC Example Vectors
aes_test \
	"7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2" \
	"--cbc" "--key" "0x2b7e151628aed2a6abf7158809cf4f3c" \
	"--iv" "0x000102030405060708090a0b0c0d0e0f" \
	"0x6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51" \
	|| exit $?
aes_test \
	"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51" \
	"--invcbc" "--key" "0x2b7e151628aed2a6abf7158809cf4f3c" \
	"--iv" "0x000102030405060708090a0b0c0d0e0f" \
	"0x7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b2" \
	|| exit $?

# F.5.1 CTR-AES128
aes_test \
	"874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee" \
	"--ctr" "--key" "0x2b7e151628aed2a6abf7158809cf4f3c" \
	"--iv" "0xf0f1f2f3f4f5f6f7f8f9fafbfcfdfeff" \
	"0x6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710" \
	|| exit $?
aes_test \
	"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710" \
	"--invctr" "--key" "0x2b7e151628aed2a6abf7158809cf4f3c" \
	"--iv" "0xf0f1f2f3f4f5f6f7f8f9fafbfcfdfeff" \
	"0x874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee" \
	|| exit $?

# nistspecialpublication800-38c.pdf
# C.1 Example 1
aes_test \
	"7162015b4dac255d" \
	--ccm --Tlen 32 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 141516' \
	--adata '0x00010203 04050607' \
	"0x20212223" \
	|| exit $?
aes_test \
	"20212223" \
	--invccm --Tlen 32 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 141516' \
	--adata '0x00010203 04050607' \
	"0x7162015b4dac255d" \
	|| exit $?

# C.2 Example 2
aes_test \
	"d2a1f0e051ea5f62081a7792073d593d1fc64fbfaccd" \
	--ccm --Tlen 48 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 14151617' \
	--adata '0x00010203 0405060708090a0b 0c0d0e0f' \
	'0x20212223 24252627 28292a2b 2c2d2e2f' \
	|| exit $?
aes_test \
	"202122232425262728292a2b2c2d2e2f" \
	--invccm --Tlen 48 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 14151617' \
	--adata '0x00010203 0405060708090a0b 0c0d0e0f' \
	'0xd2a1f0e051ea5f62081a7792073d593d1fc64fbfaccd' \
	|| exit $?

# C.3 Example 3
aes_test \
	"e3b201a9f5b71a7a9b1ceaeccd97e70b6176aad9a4428aa5484392fbc1b09951" \
	--ccm --Tlen 64 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 14151617 18191a1b' \
	--adata '0x00010203 0405060708090a0b 0c0d0e0f 10111213' \
	'0x20212223 24252627 28292a2b 2c2d2e2f 30313233 34353637' \
	|| exit $?
aes_test \
	"202122232425262728292a2b2c2d2e2f3031323334353637" \
	--invccm --Tlen 64 \
	--key '0x40414243 44454647 48494a4b 4c4d4e4f' \
	--nonce '0x10111213 14151617 18191a1b' \
	--adata '0x00010203 0405060708090a0b 0c0d0e0f 10111213' \
	'0xe3b201a9f5b71a7a9b1ceaeccd97e70b6176aad9a4428aa5484392fbc1b09951' \
	|| exit $?

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
static int aes_key_unwrap(AES_KEY *key, const uint8_t *in, size_t inlen,
			  uint8_t *out)
{
	return AES_unwrap_key(key, NULL, out, in, inlen);
}

/*
 * @out: Ciphertext. Minimal buffer length = (inlen + 8) bytes.
 * Return 0 if failed or the out length
 */
static int aes_key_wrap(AES_KEY *key, const uint8_t *in, size_t inlen,
			uint8_t *out)
{
	return AES_wrap_key(key, NULL, out, in, inlen);
}

/* Return the out length */
static int aes_cipher(AES_KEY *key, const uint8_t *in, size_t inlen,
		      uint8_t *out)
{
	AES_encrypt(in, out, key);

	return inlen;
}

static int aes_invcipher(AES_KEY *key, const uint8_t *in, size_t inlen,
			 uint8_t *out)
{
	AES_decrypt(in, out, key);

	return inlen;
}

static int aes_ecb(AES_KEY *key, const uint8_t *in, size_t inlen, uint8_t *out)
{
	for (size_t i = 0; i < inlen; i += AES_BLOCK_SIZE)
		AES_encrypt(&in[i], &out[i], key);

	return inlen;
}

static int aes_invecb(AES_KEY *key, const uint8_t *in, size_t inlen,
		      uint8_t *out)
{
	for (size_t i = 0; i < inlen; i += AES_BLOCK_SIZE)
		AES_decrypt(&in[i], &out[i], key);

	return inlen;
}

static void aes_block_data_xor(uint8_t *buf1, const uint8_t *buf2)
{
	for (size_t i = 0; i < AES_BLOCK_SIZE; i++)
		buf1[i] ^= buf2[i];
}

static void aes_block_data_add(uint8_t *in, uint8_t n)
{
	for (int i = AES_BLOCK_SIZE - 1; i > 0; i--) {
		uint16_t sum = in[i] + n;

		in[i] = sum & 0xff;
		if (sum <= 0xff)
			break;

		n = sum - 0xff;
	}
}

static void __attribute__((unused))
	aes_block_print(const uint8_t *buf, const char *prompt)
{
	printf("%20s ", prompt);
	for (size_t i = 0; i < AES_BLOCK_SIZE; i++)
		printf("%02x", buf[i]);
	printf("\n");
}

static int __aes_cbc(AES_KEY *key, const uint8_t *in, size_t inlen,
		     uint8_t *iv, uint8_t *out)
{
	uint8_t plaintext[AES_BLOCK_SIZE];

	for (size_t i = 0; i < inlen; i += AES_BLOCK_SIZE) {
		memcpy(plaintext, &in[i], AES_BLOCK_SIZE);
		aes_block_data_xor(plaintext, iv);
		AES_encrypt(plaintext, iv, key);

		if (out)
			memcpy(&out[i], iv, AES_BLOCK_SIZE);
	}

	return inlen;
}

static int aes_cbc(AES_KEY *key, const uint8_t *in, size_t inlen,
		   const uint8_t *iv, uint8_t *out)
{
	uint8_t v[AES_BLOCK_SIZE];

	memcpy(v, iv, AES_BLOCK_SIZE);

	return __aes_cbc(key, in, inlen, v, out);
}

static int aes_invcbc(AES_KEY *key, const uint8_t *in, size_t inlen,
		      const uint8_t *iv, uint8_t *out)
{
	uint8_t cipher[AES_BLOCK_SIZE], v[AES_BLOCK_SIZE];

	memcpy(v, iv, AES_BLOCK_SIZE);
	for (size_t i = 0; i < inlen; i += AES_BLOCK_SIZE) {
		memcpy(cipher, &in[i], AES_BLOCK_SIZE);

		AES_decrypt(cipher, &out[i], key);
		aes_block_data_xor(&out[i], v);

		memcpy(v, cipher, AES_BLOCK_SIZE);
	}

	return inlen;
}

static int aes_ctr(AES_KEY *key, const uint8_t *in, size_t inlen,
		   const uint8_t *iv, uint8_t *out)
{
	uint8_t counter[AES_BLOCK_SIZE], cipher[AES_BLOCK_SIZE];

	memcpy(counter, iv, AES_BLOCK_SIZE);

	for (size_t i = 0; i < inlen; i += AES_BLOCK_SIZE) {
		AES_encrypt(counter, cipher, key);
		aes_block_data_xor(cipher, &in[i]);
		memcpy(&out[i], cipher, AES_BLOCK_SIZE);

		aes_block_data_add(counter, 1);
	}

	return inlen;
}

static size_t aligned_roundup(size_t sz, size_t align)
{
	return (sz + align - 1) / align * align;
}

static void aes_ccm_gen_T(AES_KEY *key, const uint8_t *in, size_t inlen,
			  int Tlen,
			  const uint8_t *nonce, size_t nonce_sz,
			  const uint8_t *adata, size_t adata_sz,
			  uint8_t *T)
{
	uint8_t B[2048] = { 0 }, iv[AES_BLOCK_SIZE] = { 0 };
	size_t block_size = 0;
	uint8_t *B_payload;
	int t, q, n;

	/* t: the octet length of the mac
	 * q: the cotet length of the binary representation of the octet length
	 *    of the payload
	 * n: the octet length of the nonce
	 *
	 * A.1 Length Requirements
	 * t is an element of {4, 6, 8, 10, 12, 14, 16}
	 * q is an element of {2, 3, 4, 5, 6, 7, 8}
	 * n is an element of {7, 8, 9, 10, 11, 12, 13}
	 * n + q = 15
	 */
	t = Tlen / 8;
	n = nonce_sz;
	q = 15 - n;

	/* Based on 6.1 Generation-Encryption Process
	 * Steps:
	 * 1. Apply the formatting function to (N, A, P) to produce the blocks
	 * B0, B1, ... Bn
	 */

	/* A.2.1 Formattting of the Control Information and the Nonce */
	B[0] = ((!!adata_sz) << 6) | (((t - 2) / 2) << 3) | (q - 1);
	memcpy(&B[1], nonce, nonce_sz);
	B[15] = (inlen >> 0) & 0xff;
	B[14] = (inlen >> 8) & 0xff;
	if (q > 2)
		B[13] = (inlen >> 16) & 0xff;
	if (q > 3)
		B[12] = (inlen >> 24) & 0xff;
	block_size += 16;

	/* A.2.2 Formatting of the Associated Data */
	if (adata_sz > 0) {
		/* If 0 < a < 2^16 - 2^8, then a is encoded as two octets
		 * We only support this mode now.
		 */
		size_t aligned_sz =
			aligned_roundup(2 + adata_sz, AES_BLOCK_SIZE);
		uint8_t *B1 = &B[16];

		B1[0] = (adata_sz >> 8) & 0xff;
		B1[1] = (adata_sz >> 0) & 0xff;

		memcpy(&B1[2], adata, adata_sz);
		B_payload = B1 + aligned_sz;
		block_size += aligned_sz;
	} else {
		B_payload = &B[16];
	}

	/* padding payload */
	memcpy(B_payload, in, inlen);
	block_size += aligned_roundup(inlen, AES_BLOCK_SIZE);

	/* 2. Set Y0 - CIPH(B0)
	 * 3. For i = 1 to r, do Yi = CIPH(Bi ^ Yi-1)
	 * 4. Set T=MSB.Tlen(Yr)
	 */
	__aes_cbc(key, B, block_size, iv, NULL);
	memcpy(T, iv, t);
}

static int aes_ccm(AES_KEY *key, const uint8_t *in, size_t inlen, int Tlen,
		   const uint8_t *nonce, size_t nonce_sz,
		   const uint8_t *adata, size_t adata_sz,
		   uint8_t *out)
{
	uint8_t ctr[AES_BLOCK_SIZE] = { 0 }, S0[AES_BLOCK_SIZE];
	uint8_t T[AES_BLOCK_SIZE];
	int t = Tlen / 8, q = 15 - nonce_sz;

	aes_ccm_gen_T(key, in, inlen, Tlen, nonce, nonce_sz,
		      adata, adata_sz, T);

	/* 5. Apply the counter generation function to generate the counter
	 * blocks Ctr0, Ctr1, ... Ctrm, where m = [inlen / 16]
	 *
	 * CTR is based on A.3 Formattting of the Counter Blocks
	 */
	memcpy(&ctr[1], nonce, nonce_sz);
	ctr[0] |= (q - 1);

	/* 6. For j = 0 to m, do Sj = CIPH(Ctrj)
	 * 7. Set S = S1 || S2 || ... Sm
	 * 8. Return C=(P ^ MSB.Plen(S)) || (T ^ MSB.Tlen(S0))
	 */
	AES_encrypt(ctr, S0, key);
	aes_block_data_add(ctr, 1);

	aes_ctr(key, in, aligned_roundup(inlen, AES_BLOCK_SIZE), ctr, out);

	memcpy(&out[inlen], T, t);
	aes_block_data_xor(&out[inlen], S0);

	return inlen + t;
}

static int aes_invccm(AES_KEY *cipher_key,
		      const uint8_t *in, size_t inlen, int Tlen,
		      const uint8_t *nonce, size_t nonce_sz,
		      const uint8_t *adata, size_t adata_sz,
		      uint8_t *out)
{
	uint8_t ctr[AES_BLOCK_SIZE] = { 0 }, S0[AES_BLOCK_SIZE];
	uint8_t T[AES_BLOCK_SIZE];
	int t = Tlen / 8, q = 15 - nonce_sz;
	size_t plaintext_sz = inlen - t;

	/* Step1: If Clen <= Tlen, then return INVALID */
	if (inlen <= t)
		return -1;

	ctr[0] = (q - 1);
	memcpy(&ctr[1], nonce, nonce_sz);
	AES_encrypt(ctr, S0, cipher_key);
	aes_block_data_add(ctr, 1);

	/* The decrypt work is done after AES-CTR */
	aes_ctr(cipher_key, in, aligned_roundup(plaintext_sz, AES_BLOCK_SIZE),
		ctr, out);

	/* And then verify MAC by using the plaintext */
	aes_ccm_gen_T(cipher_key, out, plaintext_sz, Tlen, nonce, nonce_sz,
		      adata, adata_sz, T);
	aes_block_data_xor(T, S0);

	return memcmp(&in[inlen - t], T, t) ? -1 /* failed */ : plaintext_sz;
}

enum aes_action {
	AES_KEY_WRAP,
	AES_KEY_UNWRAP,
	AES_CIPHER,
	AES_INVCIPHER,
	AES_ECB,
	AES_INVECB,
	AES_CBC,
	AES_INVCBC,
	AES_CTR,
	AES_INVCTR,
	AES_CCM,
	AES_INVCCM,

	AES_ACTION_MAX,
};

static struct option long_options[] = {
	/* name		has_arg,		*flag,	val */
	{ "help",	no_argument,		NULL,	'h'		},
	{ "key",	required_argument,	NULL,	'K'		},
	{ "iv",		required_argument,	NULL,	'i'		},
	{ "nonce",	required_argument,	NULL,	'n'		},
	{ "adata",	required_argument,	NULL,	'A'		},
	{ "Tlen",	required_argument,	NULL,	'T'		},
	{ "wrap",	no_argument,		NULL,	AES_KEY_WRAP	},
	{ "unwrap",	no_argument,		NULL,	AES_KEY_UNWRAP	},
	{ "cipher",	no_argument,		NULL,	AES_CIPHER	},
	{ "invcipher",	no_argument,		NULL,	AES_INVCIPHER	},
	{ "ecb",	no_argument,		NULL,	AES_ECB		},
	{ "invecb",	no_argument,		NULL,	AES_INVECB	},
	{ "cbc",	no_argument,		NULL,	AES_CBC		},
	{ "invcbc",	no_argument,		NULL,	AES_INVCBC	},
	{ "ctr",	no_argument,		NULL,	AES_CTR		},
	{ "invctr",	no_argument,		NULL,	AES_INVCTR	},
	{ "ccm",	no_argument,		NULL,	AES_CCM		},
	{ "invccm",	no_argument,		NULL,	AES_INVCCM	},
	{ NULL,		0,			NULL,	0  		},
};

static void aes_usage(void)
{
	fprintf(stderr, "Usage: aes.out [OPTIONS] string1\n");
	fprintf(stderr, " -h --help:                   Show this informations\n");
	fprintf(stderr, "    --key key:                Key string\n");
	fprintf(stderr, "    --iv iv                   Set Initialization vector\n");
	fprintf(stderr, "    --nonce n:                Set the nonce number\n");
	fprintf(stderr, "    --adata data:             Set the associated data string\n");
	fprintf(stderr, "    --Tlen len:               Set the bits of the MAC\n");
	fprintf(stderr, "    --wrap:                   AES key wrap\n");
	fprintf(stderr, "    --unwrap:                 AES key unwrap\n");
	fprintf(stderr, "    --cipher:                 AES cipher plaintext\n");
	fprintf(stderr, "    --invcipher:              AES inverse cipher\n");
	fprintf(stderr, "    --ecb:                    AES-ECB\n");
	fprintf(stderr, "    --invecb:                 AES-ECB inverse\n");
	fprintf(stderr, "    --cbc:                    AES-CBC\n");
	fprintf(stderr, "    --invcbc:                 AES-CBC inverse\n");
	fprintf(stderr, "    --ctr:                    AES-CTR\n");
	fprintf(stderr, "    --invctr:                 AES-CTR inverse\n");
	fprintf(stderr, "    --ccm:                    AES-CCM\n");
	fprintf(stderr, "    --invccm:                 AES-CCM inverse\n");
	fprintf(stderr, "The input string can be hex number if starting with 0x\n");
}

int main(int argc, char **argv)
{
	uint8_t input[1024], out[sizeof(input) + 8], iv[64], key[64];
	uint8_t adata[1024], nonce[64];
	size_t input_sz = 0, key_sz = 0, iv_sz = 0, adata_sz = 0, nonce_sz = 0;
	int Tlen = 0;
	enum aes_action action = -1;
	AES_KEY cipher_key, invcipher_key;
	int ret = -1;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;

		if (c >= 0 && c < AES_ACTION_MAX) {
			action = c;
			continue;
		}

		switch (c) {
		case 'h':
			aes_usage();
			return 0;
		case 'K': /* --key */
			key_sz = read_string(key, sizeof(key), optarg);
			if (!key_sz)
				return -1;
			break;
		case 'i':
			iv_sz = read_string(iv, sizeof(iv), optarg);
			if (!iv_sz)
				return -1;
			if (iv_sz != AES_BLOCK_SIZE) {
				fprintf(stderr, "IV size should be %d\n",
					AES_BLOCK_SIZE);
				return -1;
			}
			break;
		case 'A': /* adata */
			adata_sz = read_string(adata, sizeof(adata), optarg);
			if (!adata_sz)
				return -1;
			break;
		case 'n': /* nonce */
			nonce_sz = read_string(nonce, sizeof(nonce), optarg);
			if (!nonce_sz)
				return -1;

			/* nistspecialpublication800-38c.pdf
			 * A.1 Length Requirements
			 * n is an element of {7, 8, 9, 10, 11, 12, 13}
			 */
			if (nonce_sz < 7 || nonce_sz > 13) {
				fprintf(stderr, "Nonce size is an element of "
						"{7...13}\n");
				return -1;
			}
			break;
		case 'T': /* Tlen */
			Tlen = atoi(optarg);
			switch (Tlen) {
			case 4 * 8:
			case 6 * 8:
			case 8 * 8:
			case 10 * 8:
			case 12 * 8:
			case 14 * 8:
			case 16 * 8:
				break;
			default:
				fprintf(stderr, "t is an element of "
					"{4, 6, 8, 10, 12, 14, 16}\n");
				return -1;
			}
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

	ret = AES_set_encrypt_key(key, key_sz * 8 /* to bits */, &cipher_key);
	if (ret < 0) {
		fprintf(stderr, "Bad AES key\n");
		return ret;
	}

	AES_set_decrypt_key(key, key_sz * 8, &invcipher_key);

	/* check input_sz */
	switch (action) {
	case AES_CIPHER:
	case AES_INVCIPHER:
		if (input_sz != AES_BLOCK_SIZE) {
			fprintf(stderr, "The input string size should be %d\n",
				AES_BLOCK_SIZE);
			return -1;
		}
		break;
	case AES_ECB:
	case AES_INVECB:
	case AES_CBC:
	case AES_INVCBC:
	case AES_CTR:
	case AES_INVCTR:
		if (input_sz % AES_BLOCK_SIZE) {
			fprintf(stderr, "The input string is not block size"
					" aligned\n");
			return -1;
		}
	case AES_CCM:
		/* the ccm doesn't request block size aligned.
		 * it will padding zero
		 */
		break;
	default:
		break;
	}

	/* check iv */
	switch (action) {
	case AES_CBC:
	case AES_INVCBC:
	case AES_CTR:
	case AES_INVCTR:
		if (iv_sz == 0) {
			fprintf(stderr, "--iv is not input\n");
			return -1;
		}
		break;
	case AES_CCM:
		if (nonce_sz == 0) {
			fprintf(stderr, "CCM need --nonce\n");
			return -1;
		} else if (Tlen == 0) {
			fprintf(stderr, "CCM need --Tlen\n");
			return -1;
		}

		/* Adata is optional in CCM */
		break;
	default:
		break;
	}

	switch (action) {
	case AES_KEY_UNWRAP:
		ret = aes_key_unwrap(&invcipher_key, input, input_sz, out);
		break;
	case AES_KEY_WRAP:
		ret = aes_key_wrap(&cipher_key, input, input_sz, out);
		break;
	case AES_CIPHER:
		ret = aes_cipher(&cipher_key, input, input_sz, out);
		break;
	case AES_INVCIPHER:
		ret = aes_invcipher(&invcipher_key, input, input_sz, out);
		break;
	case AES_ECB:
		ret = aes_ecb(&cipher_key, input, input_sz, out);
		break;
	case AES_INVECB:
		ret = aes_invecb(&invcipher_key, input, input_sz, out);
		break;
	case AES_CBC:
		ret = aes_cbc(&cipher_key, input, input_sz, iv, out);
		break;
	case AES_INVCBC:
		ret = aes_invcbc(&invcipher_key, input, input_sz, iv, out);
		break;
	case AES_CTR:
	case AES_INVCTR:
		ret = aes_ctr(&cipher_key, input, input_sz, iv, out);
		break;
	case AES_CCM:
		ret = aes_ccm(&cipher_key, input, input_sz,
			      Tlen,
			      nonce, nonce_sz,
			      adata, adata_sz,
			      out);
		break;
	case AES_INVCCM:
		/* Note: aes_invccm need the cipher key */
		ret = aes_invccm(&cipher_key, input, input_sz,
				 Tlen,
				 nonce, nonce_sz,
				 adata, adata_sz,
				 out);
		break;
	default:
		aes_usage();
		return -1;
	}

	/* encrypt or decrypt failed */
	if (ret <= 0)
		return -1;

	for (int i = 0; i < ret; i++)
		printf("%02x", out[i]);
	printf("\n");

	return 0;
}
