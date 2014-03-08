#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

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
	struct code *code, struct data *data)
{
	assert(f != NULL);
	assert(name != NULL);

	return var_set(&f->var, n, name, code, data);
}

struct var *
frame_get(const struct frame *f, const char *name)
{
	const struct frame *p;
	struct var *v;

	assert(f != NULL);
	assert(name != NULL);

	/* special case for $? */
	if (f->parent == NULL && 0 == strcmp(name, "?")) {
		static char s[32];
		int n;

		v = var_get(f->var, strlen(name), name);
		if (v == NULL || v->data == NULL) {
			errno = EINVAL;
			return NULL;
		}

		n = sprintf(s, "%d", status);
		if (n == -1) {
			return NULL;
		}

		v->data->s = s;

		return v;
	}

	for (p = f; p != NULL; p = p->parent) {
		v = var_get(p->var, strlen(name), name);
		if (v != NULL) {
			return v;
		}
	}

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

		if (-1 == eval_clone(v->code, v->data, &out)) {
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

