#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "lex.h"
#include "code.h"
#include "var.h"

static struct var *
var_new(struct var **v, size_t n, const char *name,
	struct code *code)
{
	struct var *new;

	assert(v != NULL);
	assert(name != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->name    = memcpy((char *) new + sizeof *new, name, n);
	new->name[n] = '\0';
	new->code    = code;

	new->next = *v;
	*v = new;

	return new;
}

void
var_replace(struct var *v, struct code *code)
{
	assert(v != NULL);
	assert(v->code == NULL || v->code != code);

	code_free(v->code);

	if (debug & DEBUG_VAR) {
		fprintf(stderr, "$%s replace: ", v->name);
		code_dump(stderr, code);
	}

	v->code = code;
}

struct var *
var_set(struct var **v, size_t n, const char *name,
	struct code *code)
{
	struct var *curr;

	assert(v != NULL);

	curr = var_get(*v, n, name);
	if (curr != NULL) {
		var_replace(curr, code);

		return curr;
	}

	if (debug & DEBUG_VAR) {
		fprintf(stderr, "$%.*s set: ", (int) n, name);
		code_dump(stderr, code);
	}

	return var_new(v, n, name, code);
}

struct var *
var_get(struct var *v, size_t n, const char *name)
{
	struct var *p;

	for (p = v; p != NULL; p = p->next) {
		if (n != strlen(p->name)) {
			continue;
		}

		if (0 != memcmp(p->name, name, n)) {
			continue;
		}

		if (debug & DEBUG_VAR) {
			fprintf(stderr, "$%.*s set: ", (int) n, name);
			code_dump(stderr, p->code);
		}

		return p;
	}

	return NULL;
}

void
var_free(struct var *v)
{
	struct var *p, *next;

	for (p = v; p != NULL; p = next) {
		next = p->next;

		code_free(p->code);
		free(p);
	}
}

