#ifndef SIGNAL_H
#define SIGNAL_H

struct frame;
struct code;

int
ss_readfd(int fd, char **s, size_t *n);

int
ss_eval(int (*eval_main)(struct frame *, struct code *),
	struct frame *top, struct code *code);

#endif

