#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "debug.h"
#include "code.h"
#include "data.h"
#include "var.h"
#include "frame.h"
#include "eval.h"

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

	/* special case for $? */
	if (f->parent == NULL && (n == 1 && 0 == memcmp(name, "?", n))) {
		static char s[32];

		v = var_get(f->var, n, name);
		if (v == NULL || v->code == NULL || v->code->type != CODE_DATA) {
			errno = EINVAL;
			return NULL;
		}

		if (-1 == sprintf(s, "%d", status)) {
			return NULL;
		}

		v->code->u.s = s;

		return v;
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

int
frame_export(const struct frame *f)
{
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
		struct data *out;
		const char *s;

		if (!isalpha((unsigned char) v->name[0])) {
			continue;
		}

		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "eval $%s for export:\n", v->name);
		}

		if (-1 == eval_clone(v->code, &out)) {
			goto error;
		}

		if (out == NULL) {
			s = "";
		} else {
			/* TODO: ^ join lists to one string (no special case for empty list) */
			/* TODO: can this merge with eval() somehow? */
			s = out->s;
		}

		if (-1 == setenv(v->name, s, 1)) {
			return -1;
		}

		data_free(out);

		continue;

error:

		data_free(out);

		return -1;
	}

	return 0;
}

