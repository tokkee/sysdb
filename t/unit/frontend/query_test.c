/*
 * SysDB - t/unit/frontend/query_test.c
 * Copyright (C) 2015 Sebastian 'tokkee' Harl <sh@tokkee.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#	include "config.h"
#endif

#include "core/memstore.h"
#include "core/plugin.h"
#include "frontend/connection.h"
#include "frontend/connection-private.h"
#include "testutils.h"

#include <check.h>

/*
 * private helpers
 */

static void
populate(void)
{
	sdb_memstore_t *store;
	sdb_data_t datum;

	/* the frontend accesses the store via the plugin API */
	store = sdb_memstore_create();
	ck_assert(store != NULL);
	ck_assert(sdb_plugin_register_writer("test-writer",
				&sdb_memstore_writer, SDB_OBJ(store)) == 0);
	ck_assert(sdb_plugin_register_reader("test-reader",
				&sdb_memstore_reader, SDB_OBJ(store)) == 0);
	sdb_object_deref(SDB_OBJ(store));

	/* populate the store */
	sdb_plugin_store_host("h1", 1 * SDB_INTERVAL_SECOND);
	sdb_plugin_store_host("h2", 3 * SDB_INTERVAL_SECOND);

	datum.type = SDB_TYPE_STRING;
	datum.data.string = "v1";
	sdb_plugin_store_attribute("h1", "k1", &datum, 1 * SDB_INTERVAL_SECOND);
	datum.data.string = "v2";
	sdb_plugin_store_attribute("h1", "k2", &datum, 2 * SDB_INTERVAL_SECOND);
	datum.data.string = "v3";
	sdb_plugin_store_attribute("h1", "k3", &datum, 2 * SDB_INTERVAL_SECOND);

	sdb_plugin_store_metric("h1", "m1", /* store */ NULL, 2 * SDB_INTERVAL_SECOND);
	sdb_plugin_store_metric("h1", "m2", /* store */ NULL, 1 * SDB_INTERVAL_SECOND);
	sdb_plugin_store_metric("h2", "m1", /* store */ NULL, 5 * SDB_INTERVAL_SECOND);
	sdb_plugin_store_metric("h2", "m1", /* store */ NULL, 10 * SDB_INTERVAL_SECOND);

	datum.type = SDB_TYPE_INTEGER;
	datum.data.integer = 42;
	sdb_plugin_store_metric_attribute("h1", "m1", "k3",
			&datum, 2 * SDB_INTERVAL_SECOND);

	sdb_plugin_store_service("h2", "s1", 1 * SDB_INTERVAL_SECOND);
	sdb_plugin_store_service("h2", "s2", 2 * SDB_INTERVAL_SECOND);

	datum.data.integer = 123;
	sdb_plugin_store_service_attribute("h2", "s2", "k1",
			&datum, 2 * SDB_INTERVAL_SECOND);
	datum.data.integer = 4711;
	sdb_plugin_store_service_attribute("h2", "s2", "k2",
			&datum, 1 * SDB_INTERVAL_SECOND);
} /* populate */

static void
turndown(void)
{
	sdb_plugin_unregister_all();
} /* turndown */

#define HOST_H1 \
	"{\"name\": \"h1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"attributes\": [" \
			"{\"name\": \"k1\", \"value\": \"v1\", " \
				"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}," \
			"{\"name\": \"k2\", \"value\": \"v2\", " \
				"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}," \
			"{\"name\": \"k3\", \"value\": \"v3\", " \
				"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h1\", " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k3\", \"value\": 42, " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}," \
			"{\"name\": \"m2\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h1\", " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}]}"
#define HOST_H2 \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:10 +0000\", " \
				"\"update_interval\": \"5s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:10 +0000\", " \
						"\"update_interval\": \"5s\", \"backends\": []}]}], " \
		"\"services\": [" \
			"{\"name\": \"s1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}," \
			"{\"name\": \"s2\", \"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k1\", \"value\": 123, " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k2\", \"value\": 4711, " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}]}"
#define HOST_H1_ARRAY "["HOST_H1"]"
#define HOST_H12_ARRAY "["HOST_H1","HOST_H2"]"
#define HOST_H1_LISTING \
	"{\"name\": \"h1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": []}"
#define HOST_H2_LISTING \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": []}"

#define SERVICE_H2_S1 \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"services\": [" \
			"{\"name\": \"s1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}]}"
#define SERVICE_H2_S12 \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"services\": [" \
			"{\"name\": \"s1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}," \
			"{\"name\": \"s2\", \"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k1\", \"value\": 123, " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k2\", \"value\": 4711, " \
						"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}]}"
#define SERVICE_H2_S1_ARRAY "["SERVICE_H2_S1"]"
#define SERVICE_H2_S12_LISTING \
	"[{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"services\": [" \
			"{\"name\": \"s1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}," \
			"{\"name\": \"s2\", \"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}]}]"

#define METRIC_H1_M1 \
	"{\"name\": \"h1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h1\", " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}," \
					"{\"name\": \"k3\", \"value\": 42, " \
						"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
						"\"update_interval\": \"0s\", \"backends\": []}]}]}"
#define METRIC_H2_M1 \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:10 +0000\", " \
				"\"update_interval\": \"5s\", \"backends\": [], " \
				"\"attributes\": [" \
					"{\"name\": \"hostname\", \"value\": \"h2\", " \
						"\"last_update\": \"1970-01-01 00:00:10 +0000\", " \
						"\"update_interval\": \"5s\", \"backends\": []}]}]}"
#define METRIC_H12_M1_ARRAY \
	"["METRIC_H1_M1","METRIC_H2_M1"]"
#define METRIC_H12_M12_LISTING \
	"[{\"name\": \"h1\", \"last_update\": \"1970-01-01 00:00:01 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:02 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}," \
			"{\"name\": \"m2\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:01 +0000\", " \
				"\"update_interval\": \"0s\", \"backends\": []}]}," \
	"{\"name\": \"h2\", \"last_update\": \"1970-01-01 00:00:03 +0000\", " \
			"\"update_interval\": \"0s\", \"backends\": [], " \
		"\"metrics\": [" \
			"{\"name\": \"m1\", \"timeseries\": false, " \
				"\"last_update\": \"1970-01-01 00:00:10 +0000\", " \
				"\"update_interval\": \"5s\", \"backends\": []}]}]"

typedef struct {
	sdb_conn_t conn;
	sdb_strbuf_t *write_buf;
} mock_conn_t;
#define MOCK_CONN(obj) ((mock_conn_t *)(obj))
#define CONN(obj) ((sdb_conn_t *)(obj))

static void
mock_conn_destroy(sdb_conn_t *conn)
{
	sdb_strbuf_destroy(conn->buf);
	sdb_strbuf_destroy(conn->errbuf);
	sdb_strbuf_destroy(MOCK_CONN(conn)->write_buf);
	free(conn);
} /* mock_conn_destroy */

static ssize_t
mock_conn_read(sdb_conn_t *conn, size_t len)
{
	if (! conn)
		return -1;
	/* unused so far */
	return len;
} /* conn_read */

static ssize_t
mock_conn_write(sdb_conn_t *conn, const void *buf, size_t len)
{
	if (! conn)
		return -1;
	return sdb_strbuf_memappend(MOCK_CONN(conn)->write_buf, buf, len);
} /* conn_write */

static sdb_conn_t *
mock_conn_create(void)
{
	mock_conn_t *conn;

	conn = calloc(1, sizeof(*conn));
	if (! conn) {
		fail("INTERNAL ERROR: failed to allocate connection object");
		return NULL;
	}

	SDB_OBJ(conn)->name = "mock_connection";
	SDB_OBJ(conn)->ref_cnt = 1;

	conn->conn.buf = sdb_strbuf_create(0);
	conn->conn.errbuf = sdb_strbuf_create(0);
	conn->write_buf = sdb_strbuf_create(64);
	if ((! conn->conn.buf) || (! conn->conn.errbuf) || (! conn->write_buf)) {
		mock_conn_destroy(CONN(conn));
		fail("INTERNAL ERROR: failed to allocate connection object");
		return NULL;
	}

	conn->conn.read = mock_conn_read;
	conn->conn.write = mock_conn_write;

	conn->conn.username = "mock_user";
	conn->conn.cmd = SDB_CONNECTION_IDLE;
	conn->conn.cmd_len = 0;
	return CONN(conn);
} /* mock_conn_create */

/* TODO: move this into a test helper module */
static void
fail_if_strneq(const char *got, const char *expected, size_t n, const char *fmt, ...)
{
	sdb_strbuf_t *buf;
	va_list ap;

	size_t len1 = strlen(got);
	size_t len2 = strlen(expected);

	size_t i;
	int pos = -1;

	if (n) {
		len1 = SDB_MIN(len1, n);
		len2 = SDB_MIN(len2, n);
	}

	if (len1 != len2)
		pos = (int)SDB_MIN(len1, len2);

	for (i = 0; i < SDB_MIN(len1, len2); ++i) {
		if (got[i] != expected[i]) {
			pos = (int)i;
			break;
		}
	}

	if (pos == -1)
		return;

	buf = sdb_strbuf_create(64);
	va_start(ap, fmt);
	sdb_strbuf_vsprintf(buf, fmt, ap);

	fail("%s\n         got: %s\n              %*s\n    expected: %s",
			sdb_strbuf_string(buf), got, pos + 1, "^", expected);
} /* fail_if_strneq */

/*
 * tests
 */

#define VALUE "\0\0\0\4""v1"
#define VALUE_LEN 7

static struct {
	uint32_t cmd;
	const char *query;
	int query_len;
	int expected;
	uint32_t code;
	uint32_t type;
	const char *data;
} query_data[] = {
	/* hosts */
	{
		SDB_CONNECTION_QUERY, "LIST hosts", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST,
		"["HOST_H1_LISTING","HOST_H2_LISTING"]",
	},
	{
		SDB_CONNECTION_LIST, "\0\0\0\1", 4,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST,
		"["HOST_H1_LISTING","HOST_H2_LISTING"]",
	},
	{
		SDB_CONNECTION_LIST, "", 0, /* LIST defaults to hosts */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST,
		"["HOST_H1_LISTING","HOST_H2_LISTING"]",
	},
	{
		SDB_CONNECTION_QUERY, "LIST hosts; LIST hosts", -1, /* ignore second (and later) commands */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST,
		"["HOST_H1_LISTING","HOST_H2_LISTING"]",
	},
	{
		SDB_CONNECTION_QUERY, "LIST hosts FILTER name = 'h1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, "["HOST_H1_LISTING"]",
	},
	{
		SDB_CONNECTION_QUERY, "LIST hosts FILTER name = 's1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, "[]",
	},
	/* SDB_CONNECTION_LIST doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH host 'h1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, HOST_H1,
	},
	{
		SDB_CONNECTION_FETCH, "\0\0\0\1""h1", 7,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, HOST_H1,
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING name = 'h1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, HOST_H1_ARRAY,
	},
	{
		SDB_CONNECTION_LOOKUP, "\0\0\0\1""name = 'h1'", 16,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, HOST_H1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH host 'h1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, HOST_H1,
	},
	/* SDB_CONNECTION_FETCH doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING name = 'h1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, HOST_H1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING ANY backend = 'b'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING ANY backend || 'b' = 'b'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, HOST_H12_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH host 'h1' FILTER age < 0s", -1, /* never matches */
		-1, UINT32_MAX, 0, NULL, /* FETCH fails if the object doesn't exist */
	},
	/* SDB_CONNECTION_FETCH doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING name = 'h1' FILTER age < 0s", -1, /* never matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	/* SDB_CONNECTION_LOOKUP doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH host 'x1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_FETCH, "\0\0\0\1x1", 7,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts MATCHING name = 'x1'", -1, /* does not exist */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_LOOKUP, "\0\0\0\1""name = 'x1'", 16, /* does not exist */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_QUERY, "FETCH host 'h1'.'s1'", -1, /* invalid args */
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP hosts BY name = 'x1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	/* services */
	{
		SDB_CONNECTION_QUERY, "LIST services", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, SERVICE_H2_S12_LISTING,
	},
	{
		SDB_CONNECTION_LIST, "\0\0\0\2", 4,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, SERVICE_H2_S12_LISTING,
	},
	{
		SDB_CONNECTION_QUERY, "LIST services FILTER host.name = 'h2'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, SERVICE_H2_S12_LISTING,
	},
	{
		SDB_CONNECTION_QUERY, "LIST services FILTER host.name = 'h1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, "[]",
	},
	/* SDB_CONNECTION_LIST doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'.'s1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, SERVICE_H2_S1,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING name = 's1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, SERVICE_H2_S1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING host.name = 'h2'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "["SERVICE_H2_S12"]",
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING ANY host.metric.name = 'm1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "["SERVICE_H2_S12"]",
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING ANY host.metric.last_update = 10s", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "["SERVICE_H2_S12"]",
	},
	{
		/* stupid but valid ;-) */
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING ANY host.host.metric.metric.interval = 5s", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "["SERVICE_H2_S12"]",
	},
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING ANY host.metric.name = 'mX'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_LOOKUP, "\0\0\0\2""name = 's1'", 16,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, SERVICE_H2_S1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'.'s1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, SERVICE_H2_S1,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING name = 's1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, SERVICE_H2_S1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'.'s1' FILTER age < 0s", -1, /* never matches */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING name = 's1' FILTER age < 0s", -1, /* never matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	/* SDB_CONNECTION_LOOKUP doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'.'s1' FILTER name = 'h2'", -1, /* only matches host */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP services MATCHING name = 's1' FILTER name = 'h2'", -1, /* only matches host */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'.'x1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "FETCH service 'x2'.'s1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	{
		SDB_CONNECTION_QUERY, "FETCH service 'h2'", -1, /* invalid args */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support services yet */
	/* metrics */
	{
		SDB_CONNECTION_QUERY, "LIST metrics", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, METRIC_H12_M12_LISTING,
	},
	{
		SDB_CONNECTION_LIST, "\0\0\0\3", 4,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, METRIC_H12_M12_LISTING,
	},
	{
		SDB_CONNECTION_QUERY, "LIST metrics FILTER age > 0s", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, METRIC_H12_M12_LISTING,
	},
	{
		SDB_CONNECTION_QUERY, "LIST metrics FILTER age < 0s", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LIST, "[]",
	},
	/* SDB_CONNECTION_LIST doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'h1'.'m1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, METRIC_H1_M1,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING name = 'm1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, METRIC_H12_M1_ARRAY,
	},
	{
		/* stupid but valid ;-) */
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING metric.metric.name = 'm1'", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, METRIC_H12_M1_ARRAY,
	},
	{
		/* also stupid but valid ;-) */
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING metric.metric.host.host.last_update = 3s", -1,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "["METRIC_H2_M1"]",
	},
	{
		SDB_CONNECTION_LOOKUP, "\0\0\0\3""name = 'm1'", 16,
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, METRIC_H12_M1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'h1'.'m1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_FETCH, METRIC_H1_M1,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING name = 'm1' FILTER age >= 0s", -1, /* always matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, METRIC_H12_M1_ARRAY,
	},
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'h1'.'m1' FILTER age < 0s", -1, /* never matches */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING name = 'm1' FILTER age < 0s", -1, /* never matches */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	/* SDB_CONEECTION_LOOKUP doesn't support filters yet */
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'h1'.'m1' FILTER name = 'h1'", -1, /* only matches host */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "LOOKUP metrics MATCHING name = 'm1' FILTER name = 'h1'", -1, /* only matches host */
		0, SDB_CONNECTION_DATA, SDB_CONNECTION_LOOKUP, "[]",
	},
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'h1'.'x1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'x1'.'m1'", -1, /* does not exist */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	{
		SDB_CONNECTION_QUERY, "FETCH metric 'x1'", -1, /* invalid args */
		-1, UINT32_MAX, 0, NULL,
	},
	/* SDB_CONNECTION_FETCH doesn't support metrics yet */
	/* timeseries */
	{
		SDB_CONNECTION_QUERY, "TIMESERIES 'h1'.'m1'", -1,
		-1, UINT32_MAX, 0, NULL, /* no data-store available */
	},
	{
		SDB_CONNECTION_QUERY, "TIMESERIES 'h1'.'x1'", -1,
		-1, UINT32_MAX, 0, NULL, /* does not exist */
	},
	{
		SDB_CONNECTION_QUERY, "TIMESERIES 'x1'.'m1'", -1,
		-1, UINT32_MAX, 0, NULL, /* does not exist */
	},
	/* store commands */
	{
		SDB_CONNECTION_QUERY, "STORE host 'hA' LAST UPDATE 01:00", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored host hA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\1""\0\0\0\0\xd6\x93\xa4\0""hA", 15,
		0, SDB_CONNECTION_OK, 0, "Successfully stored host hA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE host 'hA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored host hA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE host attribute 'h1'.'aA' 'vA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored host attribute h1.aA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x11""\0\0\0\0\xd6\x93\xa4\0""h1\0aA\0"VALUE, 18+VALUE_LEN,
		0, SDB_CONNECTION_OK, 0, "Successfully stored host attribute h1.aA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE host attribute 'x1'.'aA' 'vA'", -1,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x11""\0\0\0\0\xd6\x93\xa4\0""x1\0aA\0"VALUE, 18+VALUE_LEN,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "STORE service 'h1'.'sA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored service h1.sA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\2""\0\0\0\0\xd6\x93\xa4\0""h1\0sA", 18,
		0, SDB_CONNECTION_OK, 0, "Successfully stored service h1.sA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE service 'x1'.'sA'", -1,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x12""\0\0\0\0\xd6\x93\xa4\0""x1\0sA", 18,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "STORE service attribute 'h2'.'s1'.'aA' 'vA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored service attribute h2.s1.aA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x12""\0\0\0\0\xd6\x93\xa4\0""h2\0s1\0aA\0"VALUE,27+VALUE_LEN,
		0, SDB_CONNECTION_OK, 0, "Successfully stored service attribute h2.s1.aA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE service attribute 'h2'.'x1'.'aA' 'vA'", -1,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x12""\0\0\0\0\xd6\x93\xa4\0""h2\0x1\0aA\0"VALUE,27+VALUE_LEN,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "STORE metric 'h1'.'mA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored metric h1.mA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\3""\0\0\0\0\xd6\x93\xa4\0""h1\0mA", 18,
		0, SDB_CONNECTION_OK, 0, "Successfully stored metric h1.mA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE metric 'x1'.'mA'", -1,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\3""\0\0\0\0\xd6\x93\xa4\0""x1\0mA", 18,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_QUERY, "STORE metric attribute 'h1'.'m1'.'aA' 'vA'", -1,
		0, SDB_CONNECTION_OK, 0, "Successfully stored metric attribute h1.m1.aA",
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x13""\0\0\0\0\xd6\x93\xa4\0""h1\0m1\0aA\0"VALUE, 27+VALUE_LEN,
		0, SDB_CONNECTION_OK, 0, "Successfully stored metric attribute h1.m1.aA",
	},
	{
		SDB_CONNECTION_QUERY, "STORE metric attribute 'h1'.'x1'.'aA' 'vA'", -1,
		-1, UINT32_MAX, 0, NULL,
	},
	{
		SDB_CONNECTION_STORE, "\0\0\0\x13""\0\0\0\0\xd6\x93\xa4\0""h1\0x1\0aA\0"VALUE, 27+VALUE_LEN,
		-1, UINT32_MAX, 0, NULL,
	},
};

START_TEST(test_query)
{
	sdb_conn_t *conn = mock_conn_create();

	uint32_t code = UINT32_MAX, msg_len = UINT32_MAX;
	const char *data;
	ssize_t tmp;
	size_t len;
	int check = -1;

	conn->cmd = query_data[_i].cmd;
	if (query_data[_i].query_len < 0)
		conn->cmd_len = (uint32_t)strlen(query_data[_i].query);
	else
		conn->cmd_len = (uint32_t)query_data[_i].query_len;
	sdb_strbuf_memcpy(conn->buf, query_data[_i].query, conn->cmd_len);

	switch (conn->cmd) {
	case SDB_CONNECTION_QUERY:
		check = sdb_conn_query(conn);
		break;
	case SDB_CONNECTION_FETCH:
		check = sdb_conn_fetch(conn);
		break;
	case SDB_CONNECTION_LIST:
		check = sdb_conn_list(conn);
		break;
	case SDB_CONNECTION_LOOKUP:
		check = sdb_conn_lookup(conn);
		break;
	/* SDB_CONNECTION_TIMESERIES not supported yet */
	case SDB_CONNECTION_STORE:
		check = sdb_conn_store(conn);
		break;
	default:
		fail("Invalid command %#x", conn->cmd);
	}

	fail_unless(check == query_data[_i].expected,
			"sdb_conn_query(%s) = %d; expected: %d (err: %s)",
			query_data[_i].query, check, query_data[_i].expected,
			sdb_strbuf_string(conn->errbuf));

	data = sdb_strbuf_string(MOCK_CONN(conn)->write_buf);
	len = sdb_strbuf_len(MOCK_CONN(conn)->write_buf);

	if (query_data[_i].code == UINT32_MAX) {
		fail_unless(len == 0,
				"sdb_conn_query(%s) returned data on error: '%s'",
			query_data[_i].query, data);
		mock_conn_destroy(conn);
		return;
	}

	tmp = sdb_proto_unmarshal_header(data, len, &code, &msg_len);
	ck_assert_msg(tmp == (ssize_t)(2 * sizeof(uint32_t)));
	data += tmp;
	len -= tmp;

	fail_unless(code == query_data[_i].code,
			"sdb_conn_query(%s) returned <%u>; expected: <%u>",
			query_data[_i].query, code, query_data[_i].code);

	if (code == SDB_CONNECTION_DATA) {
		tmp = sdb_proto_unmarshal_int32(data, len, &code);
		fail_unless(code == query_data[_i].type,
				"sdb_conn_query(%s) returned %s object; expected: %s",
				query_data[_i].query, SDB_CONN_MSGTYPE_TO_STRING((int)code),
				SDB_CONN_MSGTYPE_TO_STRING((int)query_data[_i].type));
		data += tmp;
		len -= tmp;
	}

	fail_if_strneq(data, query_data[_i].data, (size_t)msg_len,
			"sdb_conn_query(%s) returned unexpected data",
			query_data[_i].query, data, query_data[_i].data);

	mock_conn_destroy(conn);
}
END_TEST

TEST_MAIN("frontend::query")
{
	TCase *tc = tcase_create("core");
	tcase_add_checked_fixture(tc, populate, turndown);
	TC_ADD_LOOP_TEST(tc, query);
	ADD_TCASE(tc);
}
TEST_MAIN_END

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

