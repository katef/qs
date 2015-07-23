#include <unistd.h>

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "frame.h"
#include "pair.h"
#include "dup.h"

int
dup_find(const struct frame *frame, int newfd)
{
	const struct pair *p;

	assert(frame != NULL);
	assert(newfd != -1);

	if (frame->parent != NULL) {
		int oldfd;

		oldfd = dup_find(frame->parent, newfd);
		if (oldfd != -1) {
			return oldfd;
		}
	}

	for (p = frame->dup; p != NULL; p = p->next) {
		if (p->n == newfd) {
			return p->m;
		}
	}

	return -1;
}

int
dup_apply(const struct frame *frame)
{
	const struct pair *p;

	assert(frame != NULL);

	/*
	 * Order matters here; innermost frames must take precedence over
	 * their parents, so it's simplest to recurr rather than iterating.
	 */

	if (frame->parent != NULL) {
		dup_apply(frame->parent);
	}

	/* XXX: perror() here from the child might be piped elsewhere */
	for (p = frame->dup; p != NULL; p = p->next) {
		assert(p->n != -1);

		if (p->m == -1) {
			if (-1 == close(p->n)) {
				perror("close");
				return -1;
			}
		} else {
			if (-1 == dup2(p->m, p->n)) {
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
	const struct pair *p;

	assert(f != NULL);
	assert(frame != NULL);

	if (frame->parent != NULL) {
		dup_dump(f, frame->parent);
	}

	for (p = frame->dup; p != NULL; p = p->next) {
		if (p->m == -1) {
			fprintf(stderr, "child: close(%d)\n", p->n);
		} else {
			fprintf(stderr, "child: dup2(%d, %d)\n", p->m, p->n);
		}
	}
}

