#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "code.h"

struct code *
code_data(struct code **head, size_t n, const char *s)
{
	struct code *new;

	assert(head != NULL);
	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->type   = CODE_DATA;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %d %.*s\n", new->type, (int) n, s);
	}

	return new;
}

struct code *
code_push(struct code **head, enum code_type type, struct frame *frame)
{
	struct code *new;

	assert(head != NULL);
	assert(frame != NULL);
	assert(type != CODE_DATA);
	assert(type && !(type & (type - 1)));

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type    = type;
	new->u.frame = frame;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %d\n", new->type);
	}

	return new;
}

struct code *
code_pop(struct code **head)
{
	struct code *node;

	assert(head != NULL);

	node = *head;
	*head = node->next;
	node->next = NULL;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %d\n", node->type);
	}

	return node;
}

void
code_free(struct code *code)
{
	struct code *p, *next;

	for (p = code; p != NULL; p = next) {
		next = p->next;

		free(p);
	}
}

struct code **
code_clone(struct code **dst, const struct code *src)
{
	const struct code *p;
	struct code **q, *end;

	assert(dst != NULL);
	assert(*dst == NULL);

	end = *dst;

	for (p = src, q = dst; p != NULL; p = p->next) {
		*q = malloc(sizeof **q);
		if (*q == NULL) {
			goto error;
		}

		if (debug & DEBUG_STACK) {
			fprintf(stderr, "code -> %d\n", p->type);
		}

		/* note .u still points into src */
		(*q)->type    = p->type;
		if (p->type == CODE_DATA) {
			(*q)->u.s     = p->u.s;
		} else {
			(*q)->u.frame = p->u.frame;
		}

		q = &(*q)->next;
	}

	*q = end;

	return q;

error:

	code_free(*dst);
	*dst = end;

	return NULL;
}

