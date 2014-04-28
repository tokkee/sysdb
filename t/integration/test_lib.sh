#
# SysDB -- t/integration/test_lib.sh
# Copyright (C) 2012 Sebastian 'tokkee' Harl <sh@tokkee.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Shell library of test helpers for integration tests.
#

TOP_SRCDIR="$( readlink -f "$( dirname "$0" )/../.." )"
TESTDIR="$( mktemp -d )"
trap "rm -rf '$TESTDIR'" EXIT

mkdir "$TESTDIR/backend"
cp "$TOP_SRCDIR/t/integration/.libs/mock_plugin.so" "$TESTDIR/backend"

SOCKET_FILE="$TESTDIR/sock"
PLUGIN_DIR="$TESTDIR"

function wait_for_sysdbd() {
	local i
	for (( i=0; i<10; i++ )); do
		if test -e "$SOCKET_FILE"; then
			break
		fi
		sleep 1
	done
	if test $i -eq 10; then
		echo 'SysDBd did not start within 10 seconds' >&2
		exit 1
	fi
}
