#include <assert.h>
#include <stdlib.h>

#include "code.h"

struct code *
code_push(struct code **head, enum code_type type, void *p)
{
	struct code *new;

	assert(head != NULL);
	assert(p != NULL);
	assert(type && !(type & (type - 1)));

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	switch (type) {
	case CODE_NULL:
	case CODE_WIND: new->u.data  = p; break;
	default:        new->u.frame = p; break;
	}

	new->type = type;
	new->next = *head;
	*head = new;

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

