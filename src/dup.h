#ifndef DUP_H
#define DUP_H

struct dup {
	int lfd;
	int rfd;

	struct dup *next;
};

int
dup_fd(const char *s, int *fd);

struct dup *
dup_push(struct dup **head, int lfd, int rfd);

void
dup_free(struct dup *d);

#endif

