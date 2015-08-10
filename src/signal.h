#ifndef SIGNAL_H
#define SIGNAL_H

const char *
signame(int s);

int
signum(const char *name);

int
ss_readfd(int fd, char **s, size_t *n);

int
sig_init(void);

int
sig_fini(void);

#endif

