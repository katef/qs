/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PROC_H
#define PROC_H

#include <sys/types.h>
#include <sys/wait.h>

enum rfork {
	RFORK_DETACH = 1 << 0
};

void
proc_exec(const char *name, char *const *argv);

pid_t
proc_wait(pid_t pid, int options);

pid_t
proc_rfork(enum rfork flags);

void
proc_exit(int r);

#endif

