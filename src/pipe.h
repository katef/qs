#ifndef PIPE_H
#define PIPE_H

struct pipe {
	int fd[2];

	struct pipe *next;
};

struct pipe *
pipe_push(struct pipe **head);

void
pipe_free(struct pipe *p);

#endif

