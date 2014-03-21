#include <assert.h>
#include <stdlib.h>

#include "debug.h"
#include "return.h"

struct rtrn *
rtrn_push(struct rtrn **head, const struct code *code)
{
	struct rtrn *new;

	assert(head != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->code = code;

	new->next = *head;
	*head = new;

	return new;
}

struct rtrn *
rtrn_pop(struct rtrn **head)
{
	struct rtrn *node;

	assert(head != NULL);

	node = *head;
	*head = node->next;
	node->next = NULL;

	return node;
}

void
rtrn_free(struct rtrn *rtrn)
{
	struct rtrn *p, *next;

	for (p = rtrn; p != NULL; p = next) {
		next = p->next;

		free(p);
	}
}

