#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

struct ast_list *
list_args(int argc, char *argv[])
{
	struct ast_list *l, **p;
	int i;

	assert(argc >= 0);
	assert(argv != NULL);

	l = NULL;

	for (i = 0, p = &l; i < argc; i++, p = &(*p)->next) {
		struct ast *a;

		a = ast_new_leaf(AST_STR, strlen(argv[i]), argv[i]);
		if (a == NULL) {
			goto error;
		}

		*p = list_cat(NULL, a);
		if (*p == NULL) {
			goto error;
		}
	}

	return l;

error:

	/* TODO: free l */

	return NULL;
}

