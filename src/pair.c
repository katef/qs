#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "pair.h"

int
pair_fd(const char *s, int *fd)
{
	char *e;
	long l;

	assert(fd != NULL);

	if (s == NULL) {
		*fd = -1;
		return 0;
	}

	l = strtol(s, &e, 10);

	if (l < INT_MIN || l > INT_MAX) {
		errno = ERANGE;
		l = LONG_MIN;
	}

	if ((l == LONG_MIN || l == LONG_MAX) && errno != 0) {
		return -1;
	}

	if (*e != '\0') {
		errno = EINVAL;
		return -1;
	}

	if (l < 0) {
		errno = EBADF;
		return -1;
	}

	*fd = l;

	return 0;
}

struct pair *
pair_push(struct pair **head, int m, int n)
{
	struct pair *new;

	assert(head != NULL);

	/* TODO: rework this to reassign over existing pairs */

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->m = m;
	new->n = n;

	new->next = *head;
	*head = new;

	return new;
}

void
pair_free(struct pair *p)
{
	struct pair *q, *next;

	for (q = p; q != NULL; q = next) {
		next = q->next;

		free(q);
	}
}

