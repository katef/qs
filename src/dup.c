#include <assert.h>
#include <stdlib.h>

#include "dup.h"

struct dup *
dup_push(struct dup **head, int lfd, int rfd)
{
	struct dup *new;

	assert(head != NULL);
	assert(lfd != -1);

	/* TODO: rework this to reassign existing fds */

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
dup_free(struct dup *d)
{
	struct dup *p, *next;

	for (p = d; p != NULL; p = next) {
		next = p->next;

		free(p);
	}
}

