#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

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

int
frame_export(const struct frame *f)
{
	const struct frame *p;
	const struct var *v;

	if (f == NULL) {
		return 0;
	}

	/*
	 * This non-tail recursion is to setenv() variables from parent frames first,
	 * and then overwrite those values by values from inner frames.
	 *
	 * Variables beginning with a non-alpha character are not exported.
	 */

	if (-1 == frame_export(f->parent)) {
		return -1;
	}

	for (v = f->var; v != NULL; v = v->next) {
		const struct ast *a;
		const char *s;

		if (!isalpha((unsigned char) v->name[0])) {
			continue;
		}

		a = v->a;

		if (a == NULL) {
			s = "";
		} else {
			/* TODO: evaulate and join lists */
			if (a->type != AST_STR) {
				continue;
			}

			s = a->u.s;
		}

		if (-1 == setenv(v->name, s, 1)) {
			return -1;
		}
	}

	return 0;
}

