#include <assert.h>
#include <stdlib.h>

#include "var.h"
#include "scope.h"

struct scope {
	struct var *var;
	struct scope *parent;
};

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
	assert(val != NULL);

	return var_set(&sc->var, name, val);
}

struct ast *
scope_get(const struct scope *sc, const char *name)
{
	const struct scope *p;
	struct ast *ast;

	assert(sc != NULL);
	assert(name != NULL);

	for (p = sc; p != NULL; p = p->parent) {
		ast = var_get(p->var, name);
		if (ast != NULL) {
			return ast;
		}
	}

	return NULL;
}

