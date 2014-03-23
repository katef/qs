#include <assert.h>
#include <stdlib.h>

#include "pipe.h"

struct pipe *
pipe_push(struct pipe **head, int lfd, int rfd)
{
	struct pipe *new;

	assert(head != NULL);
	assert(lfd != -1);
	assert(rfd != -1);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->lfd = lfd;
	new->rfd = rfd;

	new->next = *head;
	*head = new;

	return new;
}

void
pipe_free(struct pipe *p)
{
	struct pipe *q, *next;

	for (q = p; q != NULL; q = next) {
		next = q->next;

		free(q);
	}
}

