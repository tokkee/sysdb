/*
 * SysDB - t/unit/core/timeseries_test.c
 * Copyright (C) 2016 Sebastian 'tokkee' Harl <sh@tokkee.org>
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

#include "core/timeseries.h"
#include "core/data.h"
#include "testutils.h"

#include <check.h>

#include <stdbool.h>

#define TS "1970-01-01 00:00:00 +0000"
#define V "0.000000"

START_TEST(timeseries_info)
{
	const char * const data_names[] = {"abc", "xyz"};
	sdb_timeseries_info_t *ts_info = sdb_timeseries_info_create(2, data_names);

	fail_unless(ts_info != NULL,
			"sdb_timeseries_info_create(2, {\"abc\", \"xyz\"}) = NULL; expected: <ts_info>");
	sdb_timeseries_info_destroy(ts_info);
}
END_TEST

START_TEST(timeseries)
{
	const char * const data_names[] = {"abc", "xyz"};
	sdb_timeseries_t *ts = sdb_timeseries_create(2, data_names, 2);
	sdb_strbuf_t *buf = sdb_strbuf_create(0);
	int test;

	const char *expected =
		"{\"start\": \""TS"\", \"end\": \""TS"\", \"data\": {"
			"\"abc\": [{\"timestamp\": \""TS"\", \"value\": \""V"\"},"
				"{\"timestamp\": \""TS"\", \"value\": \""V"\"}],"
			"\"xyz\": [{\"timestamp\": \""TS"\", \"value\": \""V"\"},"
				"{\"timestamp\": \""TS"\", \"value\": \""V"\"}]"
		"}}";

	fail_unless(ts != NULL,
			"sdb_timeseries_create(2, {\"abc\", \"xyz\"}, 2) = NULL; expected: <ts>");

	test = sdb_timeseries_tojson(ts, buf);
	fail_unless(test == 0,
			"sdb_timeseries_tojson(<ts>, <buf>) = %d; expected: 0", test);
	sdb_diff_strings("sdb_timeseries_tojson(<ts>, <buf>) returned unexpected JSON",
			sdb_strbuf_string(buf), expected);

	sdb_timeseries_destroy(ts);
	sdb_strbuf_destroy(buf);
}
END_TEST

static struct {
	size_t initial_len;
	char **initial_names;
	size_t len;
	char **names;
	bool want_err;
} timeseries_filter_data[] = {
	/* simple combinations */
	{
		0, NULL,
		0, NULL,
		false,
	},
	{
		1, (char *[]){ "a" },
		0, NULL,
		false,
	},
	{
		1, (char *[]){ "a" },
		1, (char *[]){ "a" },
		false,
	},
	{
		2, (char *[]){ "a", "b" },
		0, NULL,
		false,
	},
	{
		2, (char *[]){ "a", "b" },
		1, (char *[]){ "a" },
		false,
	},
	{
		2, (char *[]){ "a", "b" },
		1, (char *[]){ "b" },
		false,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "b", "a" },
		false,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "a", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		0, NULL,
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		1, (char *[]){ "a" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		1, (char *[]){ "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		1, (char *[]){ "c" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "a", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "b", "a" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "b", "c" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "c", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "a", "c" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "c", "a" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "a", "b", "c" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "c", "a", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "b", "c", "a" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "c", "b", "a" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "a", "c", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		3, (char *[]){ "b", "a", "c" },
		false,
	},
	/* duplicates: these don't make sense but we want no crashes */
	{
		3, (char *[]){ "a", "a", "b" },
		3, (char *[]){ "a", "a", "b" },
		false,
	},
	{
		3, (char *[]){ "a", "b", "a" },
		3, (char *[]){ "a", "a", "b" },
		false,
	},
	{
		3, (char *[]){ "b", "a", "a" },
		3, (char *[]){ "a", "a", "b" },
		false,
	},
	/* errors */
	{
		0, NULL,
		1, (char *[]){ "a" },
		true,
	},
	{
		1, (char *[]){ "a" },
		1, (char *[]){ "b" },
		true,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "a", "c" },
		true,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "c", "b" },
		true,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "a", "a" },
		true,
	},
	{
		2, (char *[]){ "a", "b" },
		2, (char *[]){ "b", "b" },
		true,
	},
	{
		2, (char *[]){ "a", "b" },
		3, (char *[]){ "a", "b", "c" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "a", "a" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "b", "b" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "c", "c" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "a", "d" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		2, (char *[]){ "d", "a" },
		true,
	},
	{
		3, (char *[]){ "a", "b", "c" },
		1, (char *[]){ "d" },
		true,
	},
};

START_TEST(test_timeseries_filter)
{
	sdb_timeseries_t *ts;
	int status;
	size_t i;

	char initial[1024], want[1024], have[1024];

	sdb_data_format(&(sdb_data_t){
				SDB_TYPE_STRING | SDB_TYPE_ARRAY,
				{ .array = {
					timeseries_filter_data[_i].initial_len,
					timeseries_filter_data[_i].initial_names,
				} },
			}, initial, sizeof(initial), SDB_UNQUOTED);
	sdb_data_format(&(sdb_data_t){
				SDB_TYPE_STRING | SDB_TYPE_ARRAY,
				{ .array = {
					timeseries_filter_data[_i].len,
					timeseries_filter_data[_i].names,
				} },
			}, want, sizeof(want), SDB_UNQUOTED);

	ts = sdb_timeseries_create(
			timeseries_filter_data[_i].initial_len,
			(const char * const *)timeseries_filter_data[_i].initial_names, 42);
	ck_assert(ts != NULL);

	status = sdb_timeseries_filter(ts,
			timeseries_filter_data[_i].len,
			(const char * const *)timeseries_filter_data[_i].names);

	sdb_data_format(&(sdb_data_t){
				SDB_TYPE_STRING | SDB_TYPE_ARRAY,
				{ .array = { ts->data_names_len, ts->data_names } },
			}, have, sizeof(have), SDB_UNQUOTED);

	if ((status < 0) != timeseries_filter_data[_i].want_err)
		fail("sdb_timeseries_filter(ts<names=%s>, %s) = %d, names=%s; want <ERR=%d>",
				initial, want, status, have, timeseries_filter_data[_i].want_err);

	if (! timeseries_filter_data[_i].want_err) {
		for (i = 0; i < timeseries_filter_data[_i].len; i++)
			fail_unless(strcmp(ts->data_names[i], timeseries_filter_data[_i].names[i]) == 0,
					"sdb_timeseries_filter(ts<names=%s>, %s) => names=%s; want: names=%s "
					"(differs at index=%zu)", initial, want, have, want, i);

		for ( ; i < timeseries_filter_data[_i].initial_len; i++)
			fail_unless(ts->data_names[i] == NULL,
					"sdb_timeseries_filter(ts<names=%s>, %s) => index %zu not reset",
					initial, want, i);
	}

	sdb_timeseries_destroy(ts);
}
END_TEST

TEST_MAIN("core::timeseries")
{
	TCase *tc = tcase_create("core");
	tcase_add_test(tc, timeseries_info);
	tcase_add_test(tc, timeseries);
	TC_ADD_LOOP_TEST(tc, timeseries_filter);
	ADD_TCASE(tc);
}
TEST_MAIN_END

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

