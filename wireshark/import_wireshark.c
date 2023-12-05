#if 0
if ! gcc -Wall $0 -g -o import_wireshark.out ; then
        echo "compile failed"
        exit 1
fi

exit 0
#endif
/*
 * Write 802.15.4 frame in wireshark pcapng format.
 * The pcapng structure are based on wisun-br-linux project.
 *
 * qianfan Zhao <qianfanguijin@163.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>

// Link types defined in draft-richardson-opsawg-pcaplinktype
// https://datatracker.ietf.org/doc/html/draft-richardson-opsawg-pcaplinktype-00
#define LINKTYPE_IEEE802_15_4_NOFCS 230

#define PCAPNG_SHB_SIZE_MIN (        \
    4 + /* Block Type             */ \
    4 + /* Block Total Length     */ \
    4 + /* Byte-Order Magic       */ \
    2 + /* Major Version          */ \
    2 + /* Minor Version          */ \
    8 + /* Section Length         */ \
    0 + /* Options                */ \
    4   /* Block Total Length     */ \
)
#define PCAPNG_IDB_SIZE_MIN (        \
    4 + /* Block Type             */ \
    4 + /* Block Total Length     */ \
    2 + /* LinkType               */ \
    2 + /* Reserved               */ \
    4 + /* SnapLen                */ \
    0 + /* Options                */ \
    4   /* Block Total Length     */ \
)
#define PCAPNG_EPB_SIZE_MIN (        \
    4 + /* Block Type             */ \
    4 + /* Block Total Length     */ \
    4 + /* Interface ID           */ \
    4 + /* Timestamp (High)       */ \
    4 + /* Timestamp (Low)        */ \
    4 + /* Captured Packet Length */ \
    4 + /* Original Packet Length */ \
    0 + /* Packet Data            */ \
    0 + /* Options                */ \
    4   /* Block Total Length     */ \
)

struct pcapng_shb {
	uint16_t version_maj;
	uint16_t version_min;
	int64_t section_len;
	// options not supported
};

struct pcapng_idb {
	uint16_t link_type;
	uint32_t snap_len;
	// options not supported
	uint8_t *options;
	size_t option_size;
};

struct pcapng_epb {
	uint32_t if_id;
	uint64_t timestamp; // only us resolution supported (default)
	uint32_t pkt_len;
	uint32_t pkt_len_og;
	const uint8_t *pkt;
	// options not supported
};

#define PCAPNG_BLOCK_TYPE_SHB		0x0A0D0D0A
#define PCAPNG_BLOCK_TYPE_IDB		0x00000001
#define PCAPNG_BLOCK_TYPE_EPB		0x00000006

#define PCAPNG_BYTE_ORDER_MAGIC		0x1A2B3C4D

#define PCAPNG_OPT_ENDOFOPT		0

#define PCAPNG_IDB_OPTION_IF_NAME	2

struct iobuf_write {
	size_t				len;
	uint8_t				*data;
	size_t				data_size;
	int				error;
};

#define define_iobuf_write(name, sz)					\
	uint8_t iobuf_write_##name##_buffer[sz] = { 0 };		\
	struct iobuf_write name = {					\
		.data = iobuf_write_##name##_buffer,			\
		.data_size = sizeof(iobuf_write_##name##_buffer),	\
	}

static int iobuf_check_remain_size(struct iobuf_write *buf, size_t sz)
{
	size_t remain = buf->data_size - buf->len;

	/* how much more space we need? */
	if (remain < sz)
		buf->error -= (sz - remain);

	return buf->error;
}

static void iobuf_push_data(struct iobuf_write *buf, const void *data,
			    size_t len)
{
	if (!iobuf_check_remain_size(buf, len)) {
		memcpy(&buf->data[buf->len], data, len);
		buf->len += len;
	}
}

static inline void iobuf_push_u8(struct iobuf_write *buf, uint8_t val)
{
	iobuf_push_data(buf, &val, sizeof(val));
}

static inline void iobuf_push_u16(struct iobuf_write *buf, uint16_t val)
{
	iobuf_push_data(buf, &val, sizeof(val));
}

static inline void iobuf_push_u32(struct iobuf_write *buf, uint32_t val)
{
	iobuf_push_data(buf, &val, sizeof(val));
}

static inline void iobuf_push_u64(struct iobuf_write *buf, uint64_t val)
{
	iobuf_push_data(buf, &val, sizeof(val));
}

static void pcapng_write_shb(struct iobuf_write *buf, const struct pcapng_shb *shb)
{
	const uint32_t len = PCAPNG_SHB_SIZE_MIN;

	iobuf_push_u32(buf, PCAPNG_BLOCK_TYPE_SHB);
	iobuf_push_u32(buf, len);
	iobuf_push_u32(buf, PCAPNG_BYTE_ORDER_MAGIC);
	iobuf_push_u16(buf, shb->version_maj);
	iobuf_push_u16(buf, shb->version_min);
	iobuf_push_u64(buf, shb->section_len);
	// options not supported
	iobuf_push_u32(buf, len);
}

static void pcapng_write_idb(struct iobuf_write *buf, const struct pcapng_idb *idb)
{
	uint32_t len = PCAPNG_IDB_SIZE_MIN + idb->option_size;

	iobuf_push_u32(buf, PCAPNG_BLOCK_TYPE_IDB);
	iobuf_push_u32(buf, len);
	iobuf_push_u16(buf, idb->link_type);
	iobuf_push_u16(buf, 0);
	iobuf_push_u32(buf, idb->snap_len);
	iobuf_push_data(buf, idb->options, idb->option_size);
	iobuf_push_u32(buf, len);
}

static void pcapng_write_epb(struct iobuf_write *buf, const struct pcapng_epb *epb)
{
	uint8_t pkt_len_pad = (4 - (epb->pkt_len & 0b11)) & 0b11; // pad to 32 bits
	uint32_t len = PCAPNG_EPB_SIZE_MIN + epb->pkt_len + pkt_len_pad;
	uint32_t zero = 0;

	iobuf_push_u32(buf, PCAPNG_BLOCK_TYPE_EPB);
	iobuf_push_u32(buf, len);
	iobuf_push_u32(buf, epb->if_id);
	iobuf_push_u32(buf, epb->timestamp >> 32);
	iobuf_push_u32(buf, epb->timestamp & 0xffffffff);
	iobuf_push_u32(buf, epb->pkt_len);
	iobuf_push_u32(buf, epb->pkt_len_og);
	iobuf_push_data(buf, epb->pkt, epb->pkt_len);
	iobuf_push_data(buf, &zero, pkt_len_pad);
	// options not supported
	iobuf_push_u32(buf, len);
}

static struct timespec boottime;

static uint64_t get_timestamp_us(void)
{
	struct timespec now, diff;

	clock_gettime(CLOCK_MONOTONIC_RAW, &now);

	diff.tv_sec = now.tv_sec - boottime.tv_sec;
	diff.tv_nsec = now.tv_nsec - boottime.tv_nsec;
	if (diff.tv_nsec < 0) {
		diff.tv_sec--;
		diff.tv_nsec += 1000000000L;
	}

	return (uint64_t)diff.tv_sec * 1000000 + diff.tv_nsec / 1000;
}

static int pcapng_init(int fd)
{
	static const struct pcapng_shb shb = {
		.version_maj = 1,
		.version_min = 0,
		.section_len = -1, /* unknown section length */
	};

	struct pcapng_idb idb = {
		.link_type = LINKTYPE_IEEE802_15_4_NOFCS,
		.snap_len = 0, /* no packet size restriction */
	};

	define_iobuf_write(w, 256);

	clock_gettime(CLOCK_MONOTONIC_RAW, &boottime);

	pcapng_write_shb(&w, &shb);
	pcapng_write_idb(&w, &idb);
	if (w.error < 0)
		return w.error;

	write(fd, w.data, w.len);
	return 0;
}

static char pcapng_file[256];
static int get_pcapng_fd(void);

int set_pcapng_file(const char *f)
{
	int fd;

	snprintf(pcapng_file, sizeof(pcapng_file), "%s", f);

	/* try open the file first */
	fd = get_pcapng_fd();
	if (fd < 0) {
		fprintf(stderr, "open %s failed\n", pcapng_file);
		return fd;
	}

	return 0;
}

static int get_pcapng_fd(void)
{
	static int fd = -1;

	if (fd < 0) {
		if (pcapng_file[0] != '\0') {
			fd = open(pcapng_file, O_RDWR | O_CREAT, 0664);
			if (fd < 0)
				return fd;

			ftruncate(fd, 0);
		} else {
			/* use the stdout */
			fd = STDOUT_FILENO;
		}

		if (fd < 0)
			return fd;

		pcapng_init(fd);
	}

	return fd;
}

static int pcapng_write_802154_frame(uint8_t *buf, size_t sz)
{
	define_iobuf_write(w, 4096);
	struct pcapng_epb epb = { .if_id = 0 };
	int fd;

	fd = get_pcapng_fd();
	if (fd < 0)
		return fd;

	epb.timestamp = get_timestamp_us();
	epb.pkt_len_og = epb.pkt_len = sz;
	epb.pkt = buf;
	pcapng_write_epb(&w, &epb);
	if (w.error < 0)
		return w.error;

	write(fd, w.data, w.len);
	return 0;
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

static struct option long_options[] = {
	/* name		has_arg,		*flag,	val */
	{ "help",	no_argument,		NULL,	'h'		},
	{ "file",	required_argument,	NULL,	'f'		},
	{ NULL,		0,			NULL,	0  		},
};

static void usage(void)
{
	fprintf(stderr, "Usage: import_wireshark.out [OPTIONS] hex-string...\n");
	fprintf(stderr, "Options\n");
	fprintf(stderr, "-h --help:           show this help message\n");
	fprintf(stderr, "-f --file file:      write to pcapng file\n");
}

int main(int argc, char **argv)
{
	int ret = 0;

	while (1) {
		int c, option_index = 0;

		c = getopt_long(argc, argv, "hf:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'f':
			ret = set_pcapng_file(optarg);
			if (ret < 0)
				return ret;
			break;
		default:
			usage();
			return -1;
		}
	}

	for (int i = optind; i < argc; i++) {
		const char *endp;
		uint8_t buf[1024];
		size_t len;

		len = xstring(argv[optind], &endp, buf, sizeof(buf));
		if (len == 0 || *endp != '\0') {
			fprintf(stderr, "Error: bad hex string <%s>\n",
				argv[optind]);
			return -1;
		}

		pcapng_write_802154_frame(buf, len);
	}

	return 0;
}
