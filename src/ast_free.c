#include <assert.h>
#include <stdlib.h>

#include "ast.h"

void
ast_free_arg(struct ast_arg *arg)
{
	free(arg);
}

void
ast_free_exec(struct ast_exec *exec)
{
	struct ast_arg *arg, *next;

	if (exec == NULL) {
		return;
	}

	for (arg = exec->arg; arg != NULL; arg = next) {
		next = arg->next;

		ast_free_arg(arg);
	}

	free(exec);
}

void
ast_free_node(struct ast_node *node)
{
	struct ast_node *next;

	for ( ; node != NULL; node = next) {
		next = node->next;

		switch (node->type) {
		case AST_EXEC: ast_free_exec(node->u.exec); break;
		case AST_NODE: ast_free_node(node->u.node); break;

		default:
			;
		}

		free(node);
	}
}

