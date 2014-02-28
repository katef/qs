#ifndef BUILTIN_H
#define BUILTIN_H

struct frame;

int
builtin(struct frame *f, int argc, char *const *argv);

#endif

