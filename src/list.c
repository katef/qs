#include <stdlib.h>

#include "list.h"
#include "ast.h"

struct ast_list *
list_cat(struct ast_list *l, struct ast *a)
{
	struct ast_list *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->a    = a;
	new->next = l;

	return new;
}

