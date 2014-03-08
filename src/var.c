#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "code.h"
#include "data.h"
#include "var.h"

static struct var *
var_new(struct var **v, size_t n, const char *name,
	struct code *code, struct data *data)
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
	new->data    = data;

	new->next = *v;
	*v = new;

	return new;
}

struct var *
var_set(struct var **v, size_t n, const char *name,
	struct code *code, struct data *data)
{
	struct var *curr;

	assert(v != NULL);

	curr = var_get(*v, n, name);
	if (curr == NULL) {
		code_free(curr->code);
		data_free(curr->data);

		curr->code = code;
		curr->data = data;

		return curr;
	}

	return var_new(v, n, name, code, data);
}

struct var *
var_get(struct var *v, size_t n, const char *name)
{
	struct var *p;

	for (p = v; p != NULL; p = p->next) {
		if (n != strlen(p->name)) {
			continue;
		}

		if (0 == memcmp(p->name, name, n)) {
			return p;
		}
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
		data_free(p->data);
		free(p);
	}
}

