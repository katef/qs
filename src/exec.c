#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "ast.h"
#include "exec.h"
#include "builtin.h"

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

