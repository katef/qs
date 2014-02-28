#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"
#include "var.h"
#include "frame.h"

int status; /* this is $? */

struct var **
frame_push(struct frame **f)
{
	struct frame *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->var = NULL;

	new->parent = *f;
	*f = new;

	return &new->var;
}

struct frame *
frame_pop(struct frame **f)
{
	struct frame *tmp;

	assert(f != NULL);

	tmp = (*f)->parent;

	*f = tmp;

	return tmp;
}

struct var *
frame_set(struct frame *f, const char *name, struct ast *val)
{
	assert(f != NULL);
	assert(name != NULL);

	return var_set(&f->var, name, val);
}

struct ast *
frame_get(const struct frame *f, const char *name)
{
	const struct frame *p;
	struct ast *a;

	assert(f != NULL);
	assert(name != NULL);

	/* special case for $? */
	if (f->parent == NULL && 0 == strcmp(name, "?")) {
		static char s[32];
		int n;

		a = var_get(f->var, name);
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

	for (p = f; p != NULL; p = p->parent) {
		a = var_get(p->var, name);
		if (a != NULL) {
			return a;
		}
	}

	return NULL;
}

