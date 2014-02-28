#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"
#include "var.h"
#include "scope.h"

int status; /* this is $? */

struct var **
scope_push(struct scope **sc)
{
	struct scope *new;

	assert(sc != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->var = NULL;

	new->parent = *sc;
	*sc = new;

	return &new->var;
}

struct scope *
scope_pop(struct scope **sc)
{
	struct scope *tmp;

	assert(sc != NULL);

	tmp = (*sc)->parent;

	*sc = tmp;

	return tmp;
}

struct var *
scope_set(struct scope *sc, const char *name, struct ast *val)
{
	assert(sc != NULL);
	assert(name != NULL);

	return var_set(&sc->var, name, val);
}

struct ast *
scope_get(const struct scope *sc, const char *name)
{
	const struct scope *p;
	struct ast *a;

	assert(sc != NULL);
	assert(name != NULL);

	/* special case for $? */
	if (sc->parent == NULL && 0 == strcmp(name, "?")) {
		static char s[32];
		int n;

		a = var_get(sc->var, name);
		if (a == NULL) {
			errno = EINVAL;
			return NULL;
		}

		if (a->type != AST_STR) {
			errno = EINVAL;
			return NULL;
		}

		n = sprintf(s, "%d", status);
		if (n == -1) {
			return NULL;
		}

		a->u.s = s;

		return a;
	}

	for (p = sc; p != NULL; p = p->parent) {
		a = var_get(p->var, name);
		if (a != NULL) {
			return a;
		}
	}

	return NULL;
}

