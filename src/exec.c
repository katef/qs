#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "debug.h"
#include "ast.h"
#include "data.h"
#include "exec.h"
#include "builtin.h"

char **
make_argv(const struct data *data, int *argc)
{
	const struct data *p;
	char **argv;
	int i;

	assert(argc != NULL);
	assert(data != NULL);

	for (i = 0, p = data; p->s != NULL; p = p->next, i++) {
		if (p == NULL) {
			return NULL;
		}
	}

	argv = malloc((i + 1) * sizeof *argv);
	if (argv == NULL) {
		return NULL;
	}

	for (i = 0, p = data; p->s != NULL; p = p->next, i++) {
		if (p == NULL) {
			goto error;
		}

		argv[i] = p->s;
	}

	*argc = i;
	argv[i] = NULL;

	return argv;

error:

	free(argv);

	return NULL;
}

static void
dump_argv(const char *name, char **argv)
{
	int i;

	assert(name != NULL);
	assert(argv != NULL);

	fprintf(stderr, "# %s [", name);

	for (i = 0; argv[i] != NULL; i++) {
		fprintf(stderr, " \"%s\"", argv[i]);
	}

	fprintf(stderr, " ]\n");
}

int
exec_cmd(struct frame *f, int argc, char **argv)
{
	int r;

	assert(f != NULL);

	if (debug & DEBUG_EXEC) {
		dump_argv("execv", argv);
	}

	assert(argc >= 1);

	/* TODO: run pre/post-command hook here */

	r = builtin(f, argc, argv);

	return r;
}

