/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef BUILTIN_H
#define BUILTIN_H

struct task;
struct frame;

int
builtin(struct task *task, struct frame *f, int argc, char *const *argv);

#endif

