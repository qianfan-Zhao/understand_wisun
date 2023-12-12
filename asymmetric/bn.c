#if 0
#!/bin/sh

# Big number operation
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc -Wall -g -O0 $0 -lcrypto -o bn.out ; then
        echo "compile failed"
        exit 1
fi

exit 0
#endif

#include <ctype.h>
#include <string.h>
#include <openssl/bn.h>

enum {
	MATHOP_ADD,
	MATHOP_SUB,
	MATHOP_MUL,
	MATHOP_DIV,
	MATHOP_MOD,	/* % */
	MATHOP_SQR,	/* x^2 */
	MATHOP_EXP,	/* x^y */

	MATHOP_EXP_MOD,

	MATHOP_MAX,
};

static int to_mathop(const char *s)
{
	const struct mathop {
		const char *s;
		int op;
	} mathops [] = {
		{ "+",		MATHOP_ADD },
		{ "-",		MATHOP_SUB },
		{ "x",		MATHOP_MUL },
		{ "/",		MATHOP_DIV },
		{ "%",		MATHOP_MOD },
		{ "mod",	MATHOP_MOD },
		{ "sqr",	MATHOP_SQR },
		{ "exp",	MATHOP_EXP },
		{ NULL,		MATHOP_MAX },
	};

	for (const struct mathop *op = mathops; op->s; op++) {
		if (!strcmp(s, op->s))
			return op->op;
	}

	return -1;
}

static int BN_from_string(BIGNUM **bn, const char *s, int *hexmark)
{
	int hexmode = strncmp(s, "0x", 2) == 0;
	char strip[1024] = { 0 };
	size_t n = 0;
	int ret;

	if (hexmode) {
		*hexmark |= 1;
		s += 2;
	}

	for (; *s != '\0' && n < sizeof(strip) - 2; s++) {
		int good = 0;

		/* skip any spliter */
		if (isblank(*s) || *s == '\n' || *s == ':')
			continue;

		if (hexmode)
			good = isxdigit(*s);
		else
			good = isdigit(*s);

		if (!good) {
			fprintf(stderr, "Error: bad number string after\n");
			fprintf(stderr, "%s\n", s);
			return -1;
		}

		strip[n++] = *s;
	}

	if (hexmode)
		ret = BN_hex2bn(bn, strip);
	else
		ret = BN_dec2bn(bn, strip);

	return ret;
}

static int print(BIGNUM *bn, int ishex)
{
	char *p;

	if (ishex)
		p = BN_bn2hex(bn);
	else
		p = BN_bn2dec(bn);

	printf("%s%s\n", ishex ? "0x" : "", p);
	OPENSSL_free(p);

	return 0;
}

int main(int argc, char **argv)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *r = BN_new(), *rem = BN_new();
	BN_CTX *ctx = BN_CTX_new();
	int op = -1, hexmark = 0;

	if (argc < 3) {
	print_usage:
		fprintf(stderr, "Usage: bn.out \n");
		fprintf(stderr, "               a +|-|x|/|%% b\n");
		fprintf(stderr, "               a mod b\n");
		fprintf(stderr, "               a sqr\n");
		fprintf(stderr, "               a exp b mod c\n");
		return -1;
	}

	if (BN_from_string(&a, argv[1], &hexmark) <= 0)
		return -1;

	op = to_mathop(argv[2]);
	if (op < 0) {
		fprintf(stderr, "Error: badop %s\n", argv[2]);
		return op;
	}

	if (op == MATHOP_EXP) {
		/* bn.out a exp b
		 * bn.out a exp b mod c
		 */
		if (argc == 6) {
			int op2 = to_mathop(argv[4]);

			if (op2 != MATHOP_MOD)
				goto print_usage;

			if (BN_from_string(&c, argv[5], &hexmark) <= 0)
				return -1;

			op = MATHOP_EXP_MOD;
		} else if (argc != 4) {
			goto print_usage;
		}
	}

	if (op != MATHOP_SQR) {
		if (argc < 4)
			goto print_usage;

		if (BN_from_string(&b, argv[3], &hexmark) <= 0)
			return -1;
	}

	switch (op) {
	case MATHOP_ADD:
		BN_add(r, a, b);
		break;
	case MATHOP_SUB:
		BN_sub(r, a, b);
		break;
	case MATHOP_MUL:
		BN_mul(r, a, b, ctx);
		break;
	case MATHOP_DIV:
	case MATHOP_MOD:
		BN_div(r, rem, a, b, ctx);
		break;
	case MATHOP_SQR:
		BN_sqr(r, a, ctx);
		break;
	case MATHOP_EXP:
		BN_exp(r, a, b, ctx);
		break;
	case MATHOP_EXP_MOD:
		BN_mod_exp(r, a, b, c, ctx);
		break;
	}

	switch (op) {
	case MATHOP_MOD:
		print(rem, hexmark);
		break;
	default:
		print(r, hexmark);
		break;
	}

	BN_free(a);
	BN_free(r);
	BN_free(rem);
	if (b)
		BN_free(b);
	if (c)
		BN_free(c);

	BN_CTX_free(ctx);
	return 0;
}
