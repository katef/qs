#include <assert.h>
#include <stdlib.h>

#include "ast.h"

void
ast_free_list(struct ast_list *list)
{
	struct ast_list *next;

	for ( ; list != NULL; list = next) {
		next = list->next;

		free(list);
	}
}

void
ast_free_node(struct ast_node *node)
{
	struct ast_node *next;

	for ( ; node != NULL; node = next) {
		next = node->next;

		switch (node->type) {
		case AST_LIST: ast_free_list(node->u.list); break;
		case AST_NODE: ast_free_node(node->u.node); break;

		default:
			;
		}

		free(node);
	}
}

