#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "ast.h"
#include "builtin.h"

static char **
make_argv(const struct ast_exec *exec, int *argc)
{
	const struct ast_arg *arg;
	char **argv;

	assert(argc != NULL);

	for (*argc = 1, arg = exec->arg; arg != NULL; arg = arg->next, (*argc)++)
		;

	argv = malloc((*argc + 1) * sizeof *argv);
	if (argv == NULL) {
		return NULL;
	}

	argv[0] = exec->s;

	for (*argc = 1, arg = exec->arg; arg != NULL; arg = arg->next, (*argc)++) {
		argv[*argc] = arg->s;
	}

	argv[*argc] = NULL;

	return argv;
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

static int
exec_exec(struct ast_exec *exec)
{
	char **argv;
	int argc;
	int r;

	argv = make_argv(exec, &argc);
	if (argv == NULL) {
		return -1;
	}

	if (debug & DEBUG_EXEC) {
		dump_argv("execv", argv);
	}

	assert(argc >= 1);

	r = builtin(argc, argv);

	free(argv);

	return r;
}

int
exec_node(struct ast_node *node)
{
	for ( ; node != NULL; node = node->next) {
		int r;

		switch (node->type) {
		case AST_EXEC: r = exec_exec(node->u.exec); break;
		case AST_NODE: r = exec_node(node->u.node); break;

		default:
			;
		}

		if (r == -1) {
			return -1;
		}
	}

	return 0;
}

