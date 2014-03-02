#include <assert.h>
#include <stdlib.h>

#include "code.h"

struct code *
code_push(struct code *tail, enum code_type type, void *p)
{
	struct code *new;

	assert(p != NULL);
	assert(type && !(type & (type - 1)));

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	switch (type) {
	case CODE_NULL:
	case CODE_WIND: new->u.data  = p; break;
	case CODE_CALL: new->u.code  = p; break;
	default:        new->u.frame = p; break;
	}

	new->type = type;
	new->next = tail;

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

	return node;
}

