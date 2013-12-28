#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"

static char **
make_argv(const struct ast_exec *exec)
{
	const struct ast_arg *arg;
	char **argv;
	int n;

	for (n = 1, arg = exec->arg; arg != NULL; arg = arg->next, n++)
		;

	argv = malloc((n + 1) * sizeof *argv);
	if (argv == NULL) {
		return NULL;
	}

	argv[0] = exec->s;

	for (n = 1, arg = exec->arg; arg != NULL; arg = arg->next, n++) {
		argv[n] = arg->s;
	}

	argv[n] = NULL;

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

	fprintf(stderr, "]\n");
}

static int
exec_exec(struct ast_exec *exec)
{
	char **argv;
	pid_t pid;
	int status;

	pid = fork();
	switch (pid) {
	case -1:
		return -1;

	case 0:
		argv = make_argv(exec);
		if (argv == NULL) {
			return -1;
		}

		dump_argv("execv", argv);

		(void) execv(argv[0], argv);

		exit(errno);

	default:
		if (-1 == waitpid(pid, &status, 0)) {
			return -1;
		}

		fprintf(stderr, "# $?=%d\n", status);

		/* TODO: store $? */

		return 0;
	}
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

