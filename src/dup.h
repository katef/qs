#ifndef DUP_H
#define DUP_H

struct frame;

struct dup {
	int oldfd; /* -1 means to close newfd rather than dup(2) over it */
	int newfd; /* never -1 */

	struct dup *next;
};

int
dup_fd(const char *s, int *fd);

struct dup *
dup_push(struct dup **head, int oldfd, int newfd);

void
dup_free(struct dup *d);

int
dup_find(const struct frame *frame, int newfd);

int
dup_apply(const struct frame *frame);

void
dup_dump(FILE *f, const struct frame *frame);

#endif

