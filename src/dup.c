#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "frame.h"
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

	if (l < 0) {
		errno = EBADF;
		return -1;
	}

	*fd = l;

	return 0;
}

struct dup *
dup_push(struct dup **head, int oldfd, int newfd)
{
	struct dup *new;

	assert(head != NULL);
	assert(oldfd != -1);

	/* TODO: rework this to reassign existing fds */

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->oldfd = oldfd;
	new->newfd = newfd;

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

int
dup_apply(const struct frame *frame)
{
	const struct dup *p;

	/*
	 * Order matters here; innermost frames must take precedence over
	 * their parents, so it's simplest to recurr rather than iterating.
	 */

	if (frame->parent != NULL) {
		dup_apply(frame->parent);
	}

	/* XXX: perror() here from the child might be piped elsewhere */
	for (p = frame->dup; p != NULL; p = p->next) {
		assert(p->oldfd != -1);

		if (p->newfd == -1) {
			if (-1 == close(p->oldfd)) {
				perror("close");
				return -1;
			}
		} else {
			if (-1 == dup2(p->oldfd, p->newfd)) {
				perror("dup2");
				return -1;
			}
		}
	}

	return 0;
}

void
dup_dump(FILE *f, const struct frame *frame)
{
	const struct dup *p;

	assert(f != NULL);
	assert(frame != NULL);

	if (frame->parent != NULL) {
		dup_dump(f, frame->parent);
	}

	for (p = frame->dup; p != NULL; p = p->next) {
		if (p->newfd == -1) {
			fprintf(stderr, "child: close(%d)\n", p->oldfd);
		} else {
			fprintf(stderr, "child: dup2(%d, %d)\n", p->oldfd, p->newfd);
		}
	}
}

