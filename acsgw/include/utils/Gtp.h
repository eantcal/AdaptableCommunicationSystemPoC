#pragma once 

/* GTP 0 header. 
 * Explanation to some of the fields:
 * SNDCP NPDU Number flag = 0 except for inter SGSN handover situations
 * SNDCP N-PDU LCC Number 0 = 0xff except for inter SGSN handover situations
 * Sequence number. Used for reliable delivery of signalling messages, and
 *   to discard "illegal" data messages.
 * Flow label. Is used to point a particular PDP context. Is used in data
 *   messages as well as signalling messages related to a particular context.
 * Tunnel ID is IMSI+NSAPI. Unique identifier of PDP context. Is somewhat
 *   redundant because the header also includes flow. */

struct gtp0_header {		/*    Descriptions from 3GPP 09.60 */
	uint8_t flags;		/* 01 bitfield, with typical values */
	/*    000..... Version: 1 (0) */
	/*    ...1111. Spare (7) */
	/*    .......0 SNDCP N-PDU Number flag (0) */
	uint8_t type;		/* 02 Message type. T-PDU = 0xff */
	uint16_t length;	/* 03 Length (of G-PDU excluding header) */
	uint16_t seq;		/* 05 Sequence Number */
	uint16_t flow;		/* 07 Flow Label ( = 0 for signalling) */
	uint8_t number;		/* 09 SNDCP N-PDU LCC Number ( 0 = 0xff) */
	uint8_t spare1;		/* 10 Spare */
	uint8_t spare2;		/* 11 Spare */
	uint8_t spare3;		/* 12 Spare */
	uint64_t tid;		/* 13 Tunnel ID */
} __attribute__((packed));	/* 20 */

struct gtp1_header_short {	/*    Descriptions from 3GPP 29060 */
	uint8_t flags;		/* 01 bitfield, with typical values */
	/*    001..... Version: 1 */
	/*    ...1.... Protocol Type: GTP=1, GTP'=0 */
	/*    ....0... Spare = 0 */
	/*    .....0.. Extension header flag: 0 */
	/*    ......0. Sequence number flag: 0 */
	/*    .......0 PN: N-PDU Number flag */
	uint8_t type;		/* 02 Message type. T-PDU = 0xff */
	uint16_t length;	/* 03 Length (of IP packet or signalling) */
	uint32_t tei;		/* 05 - 08 Tunnel Endpoint ID */
} __attribute__((packed));

struct gtp1_header_long {	/*    Descriptions from 3GPP 29060 */
	uint8_t flags;		/* 01 bitfield, with typical values */
	/*    001..... Version: 1 */
	/*    ...1.... Protocol Type: GTP=1, GTP'=0 */
	/*    ....0... Spare = 0 */
	/*    .....0.. Extension header flag: 0 */
	/*    ......1. Sequence number flag: 1 */
	/*    .......0 PN: N-PDU Number flag */
	uint8_t type;		/* 02 Message type. T-PDU = 0xff */
	uint16_t length;	/* 03 Length (of IP packet or signalling) */
	uint32_t tei;		/* 05 Tunnel Endpoint ID */
	uint16_t seq;		/* 10 Sequence Number */
	uint8_t npdu;		/* 11 N-PDU Number */
	uint8_t next;		/* 12 Next extension header type. Empty = 0 */
} __attribute__((packed));

struct gtp0_packet {
	struct gtp0_header h;
	uint8_t p[GTP_MAX];
} __attribute__ ((packed));

struct gtp1_packet_short {
	struct gtp1_header_short h;
	uint8_t p[GTP_MAX];
} __attribute__ ((packed));

struct gtp1_packet_long {
	struct gtp1_header_long h;
	uint8_t p[GTP_MAX];
} __attribute__ ((packed));

union gtp_packet {
	uint8_t flags;
	struct gtp0_packet gtp0;
	struct gtp1_packet_short gtp1s;
	struct gtp1_packet_long gtp1l;
} __attribute__ ((packed));
