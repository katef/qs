#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "dup.h"

int
dup_fd(const char *s, int *fd)
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

	*fd = l;

	return 0;
}

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

