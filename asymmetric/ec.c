#if 0
#!/bin/sh
# qianfan Zhao <qianfanguijin@163.com>

if ! gcc -Wall -g -O0 $0 -lcrypto -o ec.out ; then
        echo "compile failed"
        exit 1
fi

sequence=1

ec_test () {
	local result expected=$1
	shift

	result=$(./ec.out "$@")
	if [ X"${result}" != X"${expected}" ] ; then
		echo "${sequence}: test failed"
		echo "${result}"
		echo "${expected}"
		return 1
	fi

	echo "${sequence}: test pass"
	let sequence++
}

prime256v1_1Gx=0x6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296
prime256v1_1Gy=0x4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5
prime256v1_2Gx=0x7CF27B188D034F7E8A52380304B51AC3C08969E277F21B35A60B48FC47669978
prime256v1_2Gy=0x07775510DB8ED040293D9AC69F7430DBBA7DADE63CE982299E04B79D227873D1
prime256v1_3Gx=0x5ECBE4D1A6330A44C8F7EF951D4BF165E6C6B721EFADA985FB41661BC6E7FD6C
prime256v1_3Gy=0x8734640C4998FF7E374B06CE1A64A2ECD82AB036384FB83D9A79B127A27D5032

ec_test \
	"($prime256v1_1Gx, $prime256v1_1Gy)" \
	prime256v1 -n 0x1 || exit $?
ec_test \
	"($prime256v1_2Gx, $prime256v1_2Gy)" \
	prime256v1 -n 0x2 || exit $?
ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -n 0x3 || exit $?

ec_test \
	"($prime256v1_1Gx, $prime256v1_1Gy)" \
	prime256v1 -m 0x1 || exit $?
ec_test \
	"($prime256v1_2Gx, $prime256v1_2Gy)" \
	prime256v1 -m 0x2 || exit $?
ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -m 0x3 || exit $?

# 1G + 1G = 2G
ec_test \
	"($prime256v1_2Gx, $prime256v1_2Gy)" \
	prime256v1 -n 0x1 -m 0x1 || exit $?

# 1G + 2G = 3G
ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -n 0x1 -m 0x2 || exit $?

ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -n 0x1 -m 0x2 -X $prime256v1_1Gx -Y $prime256v1_1Gy \
	|| exit $?

ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -n 0x1 -X $prime256v1_2Gx -Y $prime256v1_2Gy \
	|| exit $?

# 2G + 1G = 3G
ec_test \
	"($prime256v1_3Gx, $prime256v1_3Gy)" \
	prime256v1 -n 0x2 -m 0x1 || exit $?

exit 0
#endif

#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

static int BN_from_string(BIGNUM **bn, const char *s, int *hexmark)
{
	int hexmode = strncmp(s, "0x", 2) == 0;
	char strip[1024] = { 0 };
	size_t n = 0;
	int ret;

	if (hexmode) {
		if (hexmark)
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

static EC_POINT *new_ec_point_from_xy(EC_GROUP *group, const char *sx,
				      const char *sy, int *hexmark)
{
	BIGNUM *x = NULL, *y = NULL;
	EC_POINT *p = NULL;
	int ret;

	if (BN_from_string(&x, sx, hexmark) <= 0)
		return p;
	if (BN_from_string(&y, sy, hexmark) <= 0)
		return p;

	p = EC_POINT_new(group);
	if (!p)
		goto free_xy;

	ret = EC_POINT_set_affine_coordinates(group, p, x, y, NULL);
	if (ret <= 0) {
		fprintf(stderr, "(%s, %s) is not location in EC group\n",
			sx, sy);
		EC_POINT_free(p);
		p = NULL;
	}

free_xy:
	BN_free(x);
	BN_free(y);
	return p;
}

static int print_bn(BIGNUM *x, int ishex)
{
	char *p;

	if (ishex)
		p = BN_bn2hex(x);
	else
		p = BN_bn2dec(x);

	printf("%s%s", ishex ? "0x" : "", p);
	OPENSSL_free(p);

	return 0;
}

static int print_coordinates(BIGNUM *x, BIGNUM *y, int ishex)
{
	printf("(");
	print_bn(x, ishex);
	printf(", ");
	print_bn(y, ishex);
	printf(")\n");

	return 0;
}

static int print_point(EC_GROUP *group, EC_POINT *p, int ishex)
{
	BIGNUM *x = BN_new(), *y = BN_new();

	EC_POINT_get_affine_coordinates(group, p, x, y, NULL);
	print_coordinates(x, y, ishex);

	BN_free(x);
	BN_free(y);

	return 0;
}

struct ec_curve {
	int nid;
	const char *name;
};

static const struct ec_curve curves[] = {
	{
		.nid = NID_X9_62_prime256v1,
		.name = "prime256v1"
	}, {
		.nid = NID_sm2,
		.name = "sm2",
	}, {
		.name = NULL,
	}
};

static int ec_curve_find_by_name(const char *name)
{
	for (const struct ec_curve *c = curves; c->name; c++)
		if (!strcmp(name, c->name))
			return c->nid;

	return -1;
}

static struct option long_options[] = {
	/* name		has_arg,		*flag,	val */
	{ "help",	no_argument,		NULL,	'h'		},
	{ "qx",		required_argument,	NULL,	'X'		},
	{ "qy",		required_argument,	NULL,	'Y'		},
	{ NULL,		0,			NULL,	0  		},
};

static void ec_usage(void)
{
	fprintf(stderr, "Usage: ec.out curve_name [-n n] [-m m] [-qx, -qy] [curve-name]\n");
	fprintf(stderr, "       The result will be\n");
	fprintf(stderr, "              generator * n + {q} * m\n");
	fprintf(stderr, "       q is an optional, default is generator(G)\n");
}

int main(int argc, char **argv)
{
	const char *qx = NULL, *qy = NULL;
	EC_POINT *r = NULL, *q = NULL;
	BIGNUM *n = NULL, *m = NULL;
	EC_GROUP *group = NULL;
	int nid, hexmark = 0;
	int ret = -1;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "hn:m:X:Y:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			ec_usage();
			return 0;
		case 'n':
			if (BN_from_string(&n, optarg, &hexmark) <= 0)
				goto free_nm;
			break;
		case 'm':
			if (BN_from_string(&m, optarg, &hexmark) <= 0)
				goto free_nm;
			break;
		case 'X':
			qx = optarg;
			break;
		case 'Y':
			qy = optarg;
			break;
		default:
			ec_usage();
			return -1;
		}
	}

	if (!(optind < argc)) {
		ec_usage();
		goto free_nm;
	}

	nid = ec_curve_find_by_name(argv[optind]);
	if (nid < 0) {
		fprintf(stderr, "Error: Unsupported curve %s\n",
			argv[optind]);
		ret = nid;
		goto free_nm;
	}

	group = EC_GROUP_new_by_curve_name(nid);

	if (qx && !qy) {
		fprintf(stderr, "Error: --qy is not input\n");
		goto free_nm;
	} else if (!qx && qy) {
		fprintf(stderr, "Error: --qx is not input\n");
		goto free_nm;
	}

	if (qx && qy) {
		/* q = {X, Y} */
		q = new_ec_point_from_xy(group, qx, qy, &hexmark);
		if (!q)
			goto free_grp;

		if (!m)
			BN_from_string(&m, "1", NULL);
	} else {
		BIGNUM *one = NULL;

		/* q = G */
		q = EC_POINT_new(group);
		if (!q)
			goto free_grp;

		BN_from_string(&one, "1", NULL);
		EC_POINT_mul(group, q, one, NULL, NULL, NULL);
		BN_free(one);
	}

	r = EC_POINT_new(group);
	if (!r)
		goto free_point;

	/* Computes r = generator * n + q * m */
	EC_POINT_mul(group, r, n, q, m, NULL);
	ret = print_point(group, r, hexmark);

free_point:
	if (r)
		EC_POINT_free(r);
	if (q)
		EC_POINT_free(q);
free_grp:
	EC_GROUP_free(group);
free_nm:
	if (n)
		BN_free(n);
	if (m)
		BN_free(m);

	return ret;
}
