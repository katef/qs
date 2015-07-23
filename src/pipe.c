#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "pipe.h"

struct pipe *
pipe_push(struct pipe **head)
{
	struct pipe *new;
	int fd[2];

	assert(head != NULL);

	if (-1 == pipe(fd)) {
		perror("pipe");
		return NULL;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->fd[0] = fd[0];
	new->fd[1] = fd[1];

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

		if (q->fd[0] != -1) {
			if (-1 == close(q->fd[0])) {
				perror("close");
			}
		}

		if (q->fd[1] != -1) {
			if (-1 == close(q->fd[1])) {
				perror("close");
			}
		}

		free(q);
	}
}

