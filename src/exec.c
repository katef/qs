#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "debug.h"
#include "data.h"
#include "code.h"
#include "exec.h"
#include "builtin.h"

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
	assert(f != NULL);

	if (debug & DEBUG_EXEC) {
		dump_argv("execv", argv);
	}

	assert(argc >= 1);

	/* TODO: run pre/post-command hook here */

	return builtin(f, argc, argv);
}

