#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"

struct stack *
stack_push_str(struct stack *tail, size_t n, const char *s)
{
	struct stack *new;

	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	new->type = STACK_STR;
	new->next = tail;

	return new;
}

struct stack *
stack_push_op(struct stack *tail, enum stack_type type, struct frame *f)
{
	struct stack *new;

	assert(f != NULL);
	assert(type != STACK_STR);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->u.f  = f;

	new->type = type;
	new->next = tail;

	return new;
}

struct stack *
stack_pop(struct stack **head, unsigned int mask)
{
	struct stack *node;

	assert(head != NULL);

	node = *head;

	if ((node->type & mask) == 0) {
		return NULL;
	}

	*head = node->next;
	node->next = NULL;

	return node;
}

