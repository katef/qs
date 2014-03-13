#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "code.h"
#include "data.h"
#include "frame.h"
#include "status.h"
#include "builtin.h"

/*
 * Only eval.c is responsible for setting the status.
 * Return values here are an analogue of main(), and specifically:
 *
 *  -1 - means the caller should perror() if errno is non-zero
 *   0 - per EXIT_SUCCESS
 *   * - per EXIT_FAILURE or other value (errno is 0)
 */

static int
builtin_cd(struct frame *f, int argc, char *const *argv)
{
	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	if (argc != 2) {
		fprintf(stderr, "usage: cd <dir>\n");
		return 1;
	}

	(void) f;

	/* TODO: support argc=0 to chdir to $home */

	if (-1 == chdir(argv[1])) {
		perror(argv[1]);
		return 1;
	}

	return 0;
}

static int
builtin_exec(struct frame *f, int argc, char *const *argv)
{
	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	/* TODO: hook on export for setenv here */

	(void) execv(argv[0], argv);

	perror(argv[0]);

	return -1;
}

static int
builtin_exit(struct frame *f, int argc, char *const *argv)
{
	char *e;
	long r;

	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	if (argc != 2) {
		fprintf(stderr, "usage: exit <status>\n");
		return 1;
	}

	errno = 0;

	r = strtol(argv[1], &e, 10);

	if (r < INT_MIN || r > INT_MAX) {
		errno = ERANGE;
		r = LONG_MIN;
	}

	if ((r == LONG_MIN || r == LONG_MAX) && errno != 0) {
		perror("exit");
		return 1;
	}

	if (*e != '\0') {
		errno = EINVAL;
		perror("exit");
		return 1;
	}

	exit(r);

	perror(argv[0]);

	return -1;
}

static int
builtin_wait(struct frame *f, int argc, char *const *argv)
{
	int status;
	pid_t pid;

	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	if (argc != 2) {
		fprintf(stderr, "usage: wait <pid>\n");
		return 1;
	}

	(void) f;

	/* TODO: support argc=0 to wait() for all children */

	/* TODO: strtol and some error-checking */
	pid = atol(argv[1]);

	(void) execv(argv[0], argv);
	if (-1 == waitpid(pid, &status, 0)) {
		return -1;
	}

	if (WIFEXITED(status)) {
		status_exit(WEXITSTATUS(status));
		return 0;
	}

	if (WIFSIGNALED(status)) {
		status_sig(WTERMSIG(status));
		/* TODO: call a hook */
		return 0;
	}

	errno = EINVAL;
	return -1;
}

static int
builtin_fork(struct frame *f, int argc, char *const *argv)
{
	pid_t pid;

	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	if (argc > 1) {
		fprintf(stderr, "usage: fork\n");
		return 1;
	}

	(void) f;

	/* TODO: support argc=1 for various rfork-style flags */

	pid = fork();
	switch (pid) {
	case -1:
		return -1;

	case 0:
		/* TODO: store $pid = getpid() */

		return 0;

	default:
		return pid;
	}
}

/* TODO: eventually to be replaced by a shell function */
static int
builtin_spawn(struct frame *f, int argc, char *const *argv)
{
	char *fork_argv[] = { "fork", NULL };
	char *wait_argv[] = { "wait", NULL, NULL };
	pid_t pid;
	char s[128];

	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	pid = builtin_fork(f, 1, fork_argv);
	switch (pid) {
	case -1:
		return -1;

	case 0:
		(void) builtin_exec(f, argc, argv);

		exit(errno);

	default:
		sprintf(s, "%ld", (long) pid); /* XXX */

		wait_argv[1] = s;

		return builtin_wait(f, 2, wait_argv);
	}
}

static int
builtin_status(struct frame *f, int argc, char *const *argv)
{
	assert(f != NULL);
	assert(argc >= 1);
	assert(argv != NULL);

	if (argc != 1) {
		fprintf(stderr, "usage: status\n");
		return 1;
	}

	(void) f;

	return status_print(stdout);
}

int
builtin(struct frame *f, int argc, char *const *argv)
{
	size_t i;

	struct {
		const char *name;
		int (*f)(struct frame *f, int, char *const *);
	} a[] = {
		{ "cd",     builtin_cd     },
		{ "exec",   builtin_exec   },
		{ "exit",   builtin_exit   },
		{ "wait",   builtin_wait   },
		{ "fork",   builtin_fork   },
		{ "spawn",  builtin_spawn  },
		{ "status", builtin_status }
	};

	if (argc < 1) {
		errno = EINVAL;
		return -1;
	}

	assert(f != NULL);
	assert(argv != NULL);
	assert(argv[0] != NULL);

	/* TODO: bsearch */
	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(argv[0], a[i].name)) {
			return a[i].f(f, argc, argv);
		}
	}

	return builtin_spawn(f, argc, argv);
}

