#ifndef BUILTIN_H
#define BUILTIN_H

struct task;
struct frame;

int
builtin(struct task *task, struct frame *f, int argc, char *const *argv);

#endif

