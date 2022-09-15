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

PG_FUNCTION_INFO_V1(uuid_generate_v7);

/* return current unix epoch with milliseconds */
static uint64_t
current_time_ms(void)
{
	struct timespec	tp;
	clock_gettime(CLOCK_REALTIME, &tp);

	return ((tp.tv_sec * 1000) + (tp.tv_nsec / 1000000));
}

#define TIMESTAMP_LEN	6

Datum
uuid_generate_v7(PG_FUNCTION_ARGS)
{
	int			i;
	pg_uuid_t  *uuid = palloc0(sizeof(pg_uuid_t));
	uint64_t	ts;
	uint8	   *ptr;

	ts = current_time_ms();
	ptr = (uint8 *) &ts;

	for (i = 0; i < TIMESTAMP_LEN; i++)
	{
		uuid->data[i] = ptr[TIMESTAMP_LEN - 1 - i];
	}

	/* generate the remaining bytes as random (use strong generator) */
	if(!pg_strong_random(uuid->data + TIMESTAMP_LEN, UUID_LEN - TIMESTAMP_LEN))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate random values")));

	uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70;	/* time_hi_and_version */
	uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80;	/* clock_seq_hi_and_reserved */

	PG_RETURN_UUID_P(uuid);
}

