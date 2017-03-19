/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <unistd.h>

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "readbuf.h"
#include "signal.h"
#include "hook.h"

struct siglist {
	const char *name;
	int s;
};

#include "siglist.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static int self[2]; /* SIGCHLD self pipe */

static sigset_t ss_chld; /* SIGCHLD */
static sigset_t ss_old;
static struct sigaction sa, sa_old;

static volatile sig_atomic_t sigintr;

const char *
signame(int s)
{
	size_t i;

	for (i = 0; i < sizeof siglist / sizeof *siglist; i++) {
		if (siglist[i].s == s) {
			return siglist[i].name;
		}
	}

	return "?";
}

int
signum(const char *name)
{
	size_t i;

	for (i = 0; i < sizeof siglist / sizeof *siglist; i++) {
		if (0 == strcmp(siglist[i].name, name)) {
			return siglist[i].s;
		}
	}

	return 0;
}

/* signal handler for all signals */
static void
sighandle(int s)
{
	unsigned char n;
	int r;

	assert(s != 0);
	assert(!sigintr);

	/* just for debugging */
	if (debug & DEBUG_SIG) {
		fprintf(stderr, "%s!\n", signame(s));
	}

	n = s;

	sigintr = 1;

	do {
		r = write(self[1], &n, sizeof n);
		if (r == -1 && errno != EINTR) {
			abort();
		}
	} while (r != 1);
}

int
sig_register(const char *name)
{
	int s;

	assert(name != NULL);

	s = signum(name);
	if (s == 0) {
		fprintf(stderr, "unrecognised signal: %s\n", name);
		errno = ENOSYS;
		return -1;
	}

	if (debug & DEBUG_SIG) {
		fprintf(stderr, "registering for %s\n", name);
	}

	if (-1 == sigaction(s, &sa, NULL)) {
		perror("sigaction");
		goto fail;
	}

	return 0;

fail:

	abort();
}

int
ss_readbuf(int fd, char **s, size_t *n)
{
	fd_set fds;
	int max;
	int r;

	assert(fd != -1);
	assert(s != NULL);
	assert(n != NULL);

	/*
	 * There are two reasons to select here:
	 *
	 * Firstly, to avoid blocking on read() when a signal is delivered outside
	 * the call to read(), and therefore read() will not EINTR.
	 *
	 * Secondly, if read() returns for some other reason, so that we don't block
	 * on reading from the self-pipe when there're no signals to read.
	 * This could be catered for by only reading from the self pipe when #tick's
	 * read() gives EINTR, but since we need select() for the first case, I want
	 * to use the same mechanism for both.
	 */

	FD_ZERO(&fds);
	FD_SET(self[0], &fds);
	FD_SET(fd,  &fds);

	max = MAX(self[0], fd);

	if (-1 == sigprocmask(SIG_UNBLOCK, &ss_chld, NULL)) {
		perror("sigprocmask SIG_UNBLOCK");
		goto fail;
	}

	r = select(max + 1, &fds, NULL, NULL, NULL);

	if (-1 == sigprocmask(SIG_BLOCK, &ss_chld, NULL)) {
		perror("sigprocmask SIG_BLOCK");
		goto fail;
	}

	if (r == -1 && errno == EINTR) {
		goto yield;
	}

	if (r == -1) {
		return -1;
	}

	/*
	 * It doesn't matter which fd we deal with first, since the parent's
	 * self[1] will keep the pipe open, so we can't accientally read EOF
	 * before yielding to spawn the next child.
	 */

	if (FD_ISSET(self[0], &fds)) {
		unsigned char n;

		do {
			r = read(self[0], &n, sizeof n);
		} while (r == 0);
		if (r == -1) {
			return -1;
		}

		if (-1 == hook(signame(n))) {
			return -1;
		}

		/* If in is also ready, we'll deal with that on re-entry. */

		goto yield;
	}

	if (FD_ISSET(fd, &fds)) {
		if (-1 == sigprocmask(SIG_UNBLOCK, &ss_chld, NULL)) {
			perror("sigprocmask SIG_UNBLOCK");
			goto fail;
		}

		r = readbuf(fd, s, n);

		if (-1 == sigprocmask(SIG_BLOCK, &ss_chld, NULL)) {
			perror("sigprocmask SIG_BLOCK");
			goto fail;
		}

		if (r == -1 && errno == EINTR) {
			goto yield;
		}

		if (r == -1) {
			return -1;
		}

		if (r > 0 && sigintr) {
			sigintr = 0;
			goto yield;
		}

		return r;
	}

yield:

	/*
	 * read(2) could be interrupted either before reading (i.e. EINTR)
	 * or after reading (i.e. returning r > 0). The sigintr flag is used
	 * above to distinguish the second situation from an uninterrupted
	 * read to EOF, which also returns r > 0. So here we only yield when
	 * a signal is known to have occured.
	 */

	errno = EINTR;
	return -1;

fail:

	abort();
}

int
sig_init(void)
{
	if (-1 == pipe(self)) {
		perror("pipe");
		return -1;
	}

	if (debug & DEBUG_FD) {
		fprintf(stderr, "self pipe [%d,%d]\n", self[0], self[1]);
	}

	if (-1 == sigemptyset(&ss_chld)) {
		perror("sigemptyset");
		return -1;
	}

	if (-1 == sigaddset(&ss_chld, SIGCHLD)) {
		perror("sigaddset");
		return -1;
	}

	if (-1 == sigprocmask(SIG_BLOCK, &ss_chld, &ss_old)) {
		perror("sigprocmask SIG_BLOCK");
		goto fail;
	}

	sa.sa_handler = sighandle;
	sa.sa_flags   = SA_NOCLDSTOP;

	if (-1 == sig_register("sigchld")) {
		perror("sigaction");
		goto fail;
	}

	/* XXX: a non-SIGCHLD signal can be delivered here */

	return 0;

fail:

	abort();
}

int
sig_fini(void)
{
	assert(self[0] != -1);
	assert(self[1] != -1);

	/* XXX: not just SIGCHLD */
	if (sigaction(SIGCHLD, &sa_old, NULL)) {
		perror("sigaction");
		goto fail;
	}

	if (-1 == sigprocmask(SIG_SETMASK, &ss_old, NULL)) {
		perror("sigprocmask SIG_SETMASK");
		goto fail;
	}

	/* TODO: read off unhandled signals from self pipe */

	close(self[0]);
	close(self[1]);

	return 0;

fail:

	/*
	 * Failures which would leave the process in an altered state;
	 * I don't see how we could expect to recover here.
	 */

	abort();
}

