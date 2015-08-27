#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "debug.h"
#include "lex.h"
#include "code.h"
#include "data.h"
#include "var.h"
#include "frame.h"
#include "eval.h"
#include "pair.h"

struct frame *
frame_push(struct frame **f)
{
	struct frame *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->var = NULL;
	new->dup = NULL;
	new->asc = NULL;

	new->refcount = 0;

	new->parent = *f;
	*f = new;

	return new;
}

struct frame *
frame_pop(struct frame **f)
{
	struct frame *q;

	assert(f != NULL && *f != NULL);
	assert((*f)->refcount == 0);

	q = *f;
	*f = q->parent;

	return q;
}

void
frame_free(struct frame *f)
{
	assert(f != NULL);

	var_free(f->var);
	pair_free(f->dup); /* TODO: close fds? i don't like doing that here */
	pair_free(f->asc);

	free(f);
}

int
frame_refcount(struct frame *f, int delta)
{
	struct frame *p, *q;
	unsigned int lim;

	assert(delta == -1 || delta == +1);

	switch (delta) {
	case +1: lim = UINT_MAX; break;
	case -1: lim =        0; break;

	default:
		errno = EINVAL;
		return -1;
	}

	for (p = f; p != NULL; p = p->parent) {
		if (p->refcount == lim) {
			goto error;
		}

		p->refcount += delta;
	}

	return 0;

error:

	for (q = f->parent; q != p; q = q->parent) {
		q->refcount -= delta;
	}

	errno = ENOMEM;
	return -1;
}

struct var *
frame_set(struct frame *f, size_t n, const char *name,
	struct code *code)
{
	const struct frame *p;
	struct var *v;

	assert(f != NULL);
	assert(name != NULL);

	/* $.x is explicitly local */
	if (n > 0 && name[0] == '.') {
		return var_set(&f->var, n - 1, name + 1, code);
	}

	for (p = f; p != NULL; p = p->parent) {
		v = var_get(p->var, n, name);
		if (v == NULL) {
			continue;
		}

		var_replace(v, code);

		return v;
	}

	return var_set(&f->var, n, name, code);
}

struct var *
frame_get(const struct frame *f, size_t n, const char *name)
{
	const struct frame *p;
	struct var *v;

	assert(f != NULL);
	assert(name != NULL);

	/* $.x is explicitly local */
	if (n > 0 && name[0] == '.') {
		return var_get(f->var, n - 1, name + 1);
	}

	for (p = f; p != NULL; p = p->parent) {
		v = var_get(p->var, n, name);
		if (v != NULL) {
			return v;
		}
	}

	/* TODO: call "no such variable" hook */

	return NULL;
}

