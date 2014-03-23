#ifndef PIPE_H
#define PIPE_H

struct pipe {
	int lfd;
	int rfd;

	struct pipe *next;
};

int
pipe_new(struct pipe **head, int lfd, int rfd);

void
pipe_free(struct pipe *p);

#endif

