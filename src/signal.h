#ifndef SIGNAL_H
#define SIGNAL_H

struct task;

int
ss_readfd(int fd, char **s, size_t *n);

int
ss_eval(int (*eval_main)(struct task **tasks),
	struct task **tasks);

#endif

