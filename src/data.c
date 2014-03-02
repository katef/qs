#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

struct data *
data_push(struct data *tail, size_t n, const char *s)
{
	struct data *new;

	assert(s != NULL || n == 0);

	new = malloc(sizeof *new + n + (n > 0));
	if (new == NULL) {
		return NULL;
	}

	if (s != NULL) {
		new->s    = memcpy((char *) new + sizeof *new, s, n);
		new->s[n] = '\0';
	} else {
		new->s    = NULL;
	}

	new->next = tail;

	return new;
}

struct data *
data_pop(struct data **head)
{
	struct data *node;

	assert(head != NULL);

	node = *head;
	*head = node->next;
	node->next = NULL;

	return node;
}

