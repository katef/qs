#ifndef DUP_H
#define DUP_H

struct dup {
	int oldfd; /* never -1 */
	int newfd; /* -1 means to close oldfd rather than dup(2) over it */

	struct dup *next;
};

int
dup_fd(const char *s, int *fd);

struct dup *
dup_push(struct dup **head, int oldfd, int newfd);

void
dup_free(struct dup *d);

#endif

