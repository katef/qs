/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <unistd.h>

#include "debug.h"
#include "status.h"
#include "lex.h"
#include "data.h"
#include "code.h"
#include "frame.h"
#include "args.h"
#include "proc.h"

void
proc_exec(const char *name, char *const *argv)
{
	if (debug & DEBUG_EXEC) {
		dump_args("execv", argv);
	}

	execv(name, argv);
}

pid_t
proc_wait(pid_t pid, int options)
{
	int status;
	pid_t r;

	if (debug & DEBUG_PROC) {
		fprintf(stderr, "wait %ld\n", (long) pid);
	}

	r = waitpid(pid, &status, options);
	if (r == -1) {
		return -1;
	}

	if (r == 0) {
		assert(options & WNOHANG);
		return 0;
	}

	if (WIFEXITED(status)) {
		status_exit(WEXITSTATUS(status));
		return r;
	}

	if (WIFSIGNALED(status)) {
		status_sig(WTERMSIG(status));
		/* TODO: call a hook */
		return r;
	}

	errno = EINVAL;
	return -1;
}

pid_t
proc_rfork(enum rfork flags)
{
	assert(flags == 0);

	if (debug & DEBUG_PROC) {
		fprintf(stderr, "rfork %#x\n", (unsigned) flags);
	}

	/* TODO: flags for: daemonise; different process group */
	/* TODO: catch SIGCHLD */
	/* TODO: call fork hook here */

	return fork();
}

void
proc_exit(int r)
{
	if (debug & DEBUG_PROC) {
		fprintf(stderr, "exit %d\n", r);
	}

	/* TODO: call exit hook here */

	exit(r);
}

