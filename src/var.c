#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "var.h"
#include "ast.h"

struct var {
	const char *name;
	struct ast *a;
	struct var *next;
};

static struct var *
var_find(struct var *v, const char *name)
{
	struct var *p;

	for (p = v; p != NULL; p = p->next) {
		if (0 == strcmp(p->name, name)) {
			return p;
		}
	}

	return NULL;
}

static struct var *
var_new(struct var **v, const char *name, struct ast *a)
{
	struct var *new;

	assert(v != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->name = name; /* XXX: after struct */
	new->a    = a;

	return new;
}

struct var *
var_set(struct var **v, const char *name, struct ast *a)
{
	struct var *curr;

	assert(v != NULL);

	curr = var_find(*v, name);
	if (curr != NULL) {
		ast_free(curr->a);
		curr->a = a;

		return curr;
	}

	return var_new(v, name, a);
}

struct ast *
var_get(struct var *v, const char *name)
{
	struct var *p;

	p = var_find(v, name);
	if (p != NULL) {
		return p->a;
	}

	return NULL;
}

void
var_free(struct var *v)
{
	struct var *p, *next;

	for (p = v; p != NULL; p = next) {
		next = p->next;

		ast_free(p->a);
		free(p);
	}
}

