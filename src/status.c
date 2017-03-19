/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include "debug.h"
#include "status.h"

struct status status;

void
status_exit(int r)
{
	if (debug & DEBUG_EXEC) {
		fprintf(stderr, "# $?=%d\n", r);
	}

	status.r = r;
	status.s = 0;
}

void
status_sig(int s)
{
	assert(s != 0);

	if (debug & DEBUG_EXEC) {
		fprintf(stderr, "# $?=sig%d\n", s);
	}

	status.r = 1; /* all signals mean abnormal termination */
	status.s = s;
}

int
status_print(FILE *f)
{
	if (status.s != 0) {
		fprintf(f, "sig%d\n", status.s);
	} else {
		fprintf(f, "%d\n",    status.r);
	}

	return 0;
}

