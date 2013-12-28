#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"

static int
indent(unsigned n)
{
	while (n--) {
		fprintf(stderr, "\t");
	}

	return 0;
}

static int
dump_exec(const struct ast_exec *exec, unsigned n)
{
	const struct ast_arg *arg;

	assert(exec != NULL);
	assert(exec->s != NULL);

	if (-1 == indent(n)) {
		return -1;
	}

	fprintf(stderr, "exec:\n");

	if (-1 == indent(n + 1)) {
		return -1;
	}

	fprintf(stderr, "%s\n", exec->s);

	for (arg = exec->arg; arg != NULL; arg = arg->next) {
		if (-1 == indent(n + 1)) {
			return -1;
		}

		fprintf(stderr, "%s\n", arg->s);
	}

	return 0;
}

static int
dump_node(const struct ast_node *node, unsigned n)
{
	assert(node != NULL);

	if (-1 == indent(n)) {
		return -1;
	}

	fprintf(stderr, "node:\n");

	while (node != NULL) {
		int r;

		switch (node->type) {
		case AST_EXEC: r = dump_exec(node->u.exec, n + 1); break;
		case AST_NODE: r = dump_node(node->u.node, n + 1); break;

		default:
			errno = EINVAL;
			return -1;
		}

		if (-1 == r) {
			return -1;
		}

		node = node->next;
	}

	return 0;
}

int
ast_dump(const struct ast_node *node)
{
	return dump_node(node, 0);
}

