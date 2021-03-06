/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "lex.h"
#include "data.h"
#include "code.h"
#include "args.h"
#include "frame.h"

int
count_args(const struct data *data)
{
	const struct data *p;
	int i;

	for (i = 0, p = data; p->s != NULL; p = p->next, i++) {
		if (p == NULL) {
			errno = 0;
			return -1;
		}
	}

	return i;
}

char **
make_args(const struct data *data, int n)
{
	const struct data *p;
	char **args;
	int i;

	assert(n >= 0);

	args = malloc(n * sizeof *args);
	if (args == NULL) {
		return NULL;
	}

	for (i = 0, p = data; i < n; p = p->next, i++) {
		assert(p != NULL);

		args[i] = p->s;
	}

	return args;
}

int
set_args(const struct lex_mark *mark, struct frame *frame, char *args[])
{
	struct code *ci;
	int i;

	assert(mark != NULL);
	assert(frame != NULL);
	assert(args != NULL);

	ci = NULL;

	for (i = 0; args[i] != NULL; i++) {
		if (!code_data(&ci, mark, strlen(args[i]), args[i])) {
			goto error;
		}
	}

	if (!frame_set(frame, 1, "*", ci)) {
		goto error;
	}

	return 0;

error:

	code_free(ci);

	return -1;
}

void
dump_args(const char *name, char *const args[])
{
	int i;

	assert(name != NULL);
	assert(args != NULL);

	fprintf(stderr, "# %s [", name);

	for (i = 0; args[i] != NULL; i++) {
		fprintf(stderr, " \"%s\"", args[i]);
	}

	fprintf(stderr, " ]\n");
}

