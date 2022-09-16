/*-------------------------------------------------------------------------
 *
 * pg_uuid.c
 *	  generators of sequential UUID v6/7/8 as defined by IETF
 *
 *
 * Currently, this only works on PostgreSQL 10+. Adding support for older
 * releases is possible, but it would require solving a couple issues:
 *
 * 1) pg_uuid_t hidden in uuid.c (can be solved by local struct definition)
 *
 * 2) pg_strong_random not available (can fallback to random, probably)
 *
 * 3) functions defined as PARALLEL SAFE, which fails on pre-9.6 releases
 *
 *-------------------------------------------------------------------------
 */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "postgres.h"

#include "catalog/namespace.h"
#include "commands/sequence.h"
#include "utils/uuid.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(uuid_generate_v6);
PG_FUNCTION_INFO_V1(uuid_generate_v7);

/* return timestamp in hundreds of nanoseconds */
static uint64_t
current_timestamp_v6(void)
{
	struct timespec	tp;

	clock_gettime(CLOCK_REALTIME, &tp);

	/*
	 * Hundreds of nanoseconds (shift to adjust to UUID epoch, which starts
	 * at 1582-10-15 00:00:00, while Unix epoch is 1970-01-01 00:00:00).
	 */
	return (tp.tv_sec * 10000000L) + (tp.tv_nsec / 100L) + 0x01b21dd213814000;
}

/* return timestamp in milliseconds */
static uint64_t
current_timestamp_v7(void)
{
	struct timespec	tp;

	clock_gettime(CLOCK_REALTIME, &tp);

	/* milliseconds */
	return tp.tv_sec * 1000L + tp.tv_nsec / 1000000L;
}

Datum
uuid_generate_v6(PG_FUNCTION_ARGS)
{
	int			i;
	pg_uuid_t  *uuid = palloc0(sizeof(pg_uuid_t));
	uint64_t	ts;
	uint8	   *ptr;

	ts = current_timestamp_v6();


	/* do the shifting per RFC (http://gh.peabody.io/uuidv6/) */
	ts = ((ts << 4) & 0xFFFFFFFFFFFF0000) | (ts & 0x0FFF) | 0x6000;

	ts = htobe64(ts);

	ptr = (uint8 *) &ts;

	/* little endian - start from most sigificant bytes */
	for (i = 0; i < 8; i++)
		uuid->data[i] = ptr[i];

	/* generate the remaining bytes as random (use strong generator) */
	if(!pg_strong_random(uuid->data + 8, UUID_LEN - 8))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate random values")));

	/* finally, the clock_seq_hi_and_reserved (version already set). */
	// uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70;	/* time_hi_and_version */
	uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;	/* clock_seq_hi_and_reserved */

	PG_RETURN_UUID_P(uuid);
}

Datum
uuid_generate_v7(PG_FUNCTION_ARGS)
{
	int			i;
	pg_uuid_t  *uuid = palloc0(sizeof(pg_uuid_t));
	uint64_t	ts;
	uint8	   *ptr;

	ts = current_timestamp_v7();
	ptr = (uint8 *) &ts;

	/* convernt to big endian */
	ts = htobe64(ts);

	/* copy the 6 bytes (48 bits), starting from the most significant one */
	for (i = 0; i < 6; i++)
		uuid->data[i] = ptr[2 + i];

	/* generate the remaining bytes as random (use strong generator) */
	if(!pg_strong_random(uuid->data + 6, UUID_LEN - 6))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate random values")));

	/* finally, set the version etc. */
	uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70;	/* time_hi_and_version */
	uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;	/* clock_seq_hi_and_reserved */

	PG_RETURN_UUID_P(uuid);
}
