/*
 * SysDB - src/utils/proto.c
 * Copyright (C) 2013 Sebastian 'tokkee' Harl <sh@tokkee.org>
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

#include "utils/error.h"
#include "utils/proto.h"

#include <arpa/inet.h>
#include <errno.h>

#include <limits.h>

#include <string.h>
#include <unistd.h>

#include <sys/select.h>

/*
 * public API
 */

int
sdb_proto_select(int fd, int type)
{
	fd_set fds;
	fd_set *readfds = NULL;
	fd_set *writefds = NULL;
	fd_set *exceptfds = NULL;

	if (fd < 0) {
		errno = EBADF;
		return -1;
	}

	FD_ZERO(&fds);

	switch (type) {
		case SDB_PROTO_SELECTIN:
			readfds = &fds;
			break;
		case SDB_PROTO_SELECTOUT:
			writefds = &fds;
			break;
		case SDB_PROTO_SELECTERR:
			exceptfds = &fds;
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	FD_SET(fd, &fds);

	while (42) {
		int n;
		errno = 0;
		n = select(fd + 1, readfds, writefds, exceptfds, NULL);

		if ((n < 0) && (errno != EINTR))
			return n;
		if (n > 0)
			break;
	}
	return 0;
} /* sdb_proto_select */

ssize_t
sdb_proto_send(int fd, size_t msg_len, const char *msg)
{
	const char *buf;
	size_t len;

	if ((fd < 0) || (msg_len && (! msg)))
		return -1;
	if (! msg_len)
		return 0;

	buf = msg;
	len = msg_len;
	while (len > 0) {
		ssize_t status;

		if (sdb_proto_select(fd, SDB_PROTO_SELECTOUT))
			return -1;

		errno = 0;
		status = write(fd, buf, len);
		if (status < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
				continue;
			if (errno == EINTR)
				continue;

			return status;
		}

		len -= (size_t)status;
		buf += status;
	}

	return (ssize_t)msg_len;
} /* sdb_proto_send */

ssize_t
sdb_proto_send_msg(int fd, uint32_t code,
		uint32_t msg_len, const char *msg)
{
	size_t len = 2 * sizeof(uint32_t) + msg_len;
	char buffer[len];

	uint32_t tmp;

	tmp = htonl(code);
	memcpy(buffer, &tmp, sizeof(tmp));
	tmp = htonl(msg_len);
	memcpy(buffer + sizeof(tmp), &tmp, sizeof(tmp));

	if (msg_len)
		memcpy(buffer + 2 * sizeof(tmp), msg, msg_len);

	return sdb_proto_send(fd, len, buffer);
} /* sdb_proto_send_msg */

uint32_t
sdb_proto_get_int(sdb_strbuf_t *buf, size_t offset)
{
	const char *data;
	uint32_t n;

	if (! buf)
		return UINT32_MAX;

	/* not enough data to read */
	if (offset + sizeof(uint32_t) > sdb_strbuf_len(buf))
		return UINT32_MAX;

	data = sdb_strbuf_string(buf);
	data += offset;
	memcpy(&n, data, sizeof(n));
	return ntohl(n);
} /* sdb_proto_get_int */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */
