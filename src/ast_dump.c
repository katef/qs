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
dump_list(const struct ast_list *list, unsigned n)
{
	if (-1 == indent(n)) {
		return -1;
	}

	fprintf(stderr, "list:\n");

	for ( ; list != NULL; list = list->next) {
		if (-1 == indent(n + 1)) {
			return -1;
		}

		fprintf(stderr, "%s\n", list->s);
	}

	return 0;
}

static int
dump_node(const struct ast_node *node, unsigned n)
{
	if (-1 == indent(n)) {
		return -1;
	}

	fprintf(stderr, "node:\n");

	while (node != NULL) {
		int r;

		switch (node->type) {
		case AST_LIST: r = dump_list(node->u.list, n + 1); break;
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

