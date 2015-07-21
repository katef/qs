#ifndef DUP_H
#define DUP_H

struct dup {
	int oldfd;
	int newfd;

	struct dup *next;
};

int
dup_fd(const char *s, int *fd);

struct dup *
dup_push(struct dup **head, int oldfd, int newfd);

void
dup_free(struct dup *d);

#endif

