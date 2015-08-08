#ifndef SIGNAL_H
#define SIGNAL_H

struct task;

const char *
signame(int s);

int
signum(const char *name);

int
ss_readfd(int fd, char **s, size_t *n);

int
ss_eval(int (*eval_main)(struct task **tasks),
	struct task **tasks);

#endif

