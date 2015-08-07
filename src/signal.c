#define _POSIX_SOURCE

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <unistd.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "readfd.h"
#include "signal.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int self[2]; /* SIGCHLD self pipe */

static sigset_t ss_chld; /* SIGCHLD */

static volatile sig_atomic_t sigintr;

static void
sigchld(int s)
{
	const char dummy = 'x';
	int r;

	assert(s == SIGCHLD);
	assert(!sigintr);

	(void) s;

	sigintr = 1;

	do {
		r = write(self[1], &dummy, sizeof dummy);
		if (r == -1 && errno != EINTR) {
			abort();
		}
	} while (r != 1);
}

int
ss_readfd(int fd, char **s, size_t *n)
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
		char dummy;

		do {
			r = read(self[0], &dummy, sizeof dummy);
		} while (r == 0);
		if (r == -1) {
			return -1;
		}

		assert(dummy == 'x');

		/* If in is also ready, we'll deal with that on re-entry. */

		goto yield;
	}

	if (FD_ISSET(fd, &fds)) {
		if (-1 == sigprocmask(SIG_UNBLOCK, &ss_chld, NULL)) {
			perror("sigprocmask SIG_UNBLOCK");
			goto fail;
		}

		r = readfd(fd, s, n);

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
ss_eval(int (*eval_main)(struct frame *, struct code *),
	struct frame *top, struct code *code)
{
	struct sigaction sa, sa_old;
	sigset_t ss_old;
	int r;

	assert(eval_main != NULL);
	assert(top != NULL);

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
		perror("sigprocmask SIG_UNBLOCK");
		goto fail;
	}

	sa.sa_handler = sigchld;
	sa.sa_flags   = SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &sa, &sa_old)) {
		perror("sigaction");
		goto fail;
	}

	r = eval_main(top, code);
	if (r == -1 && errno != 0) {
		perror("eval");
	}

	if (sigaction(SIGCHLD, &sa_old, NULL)) {
		perror("sigaction");
		goto fail;
	}

	if (-1 == sigprocmask(SIG_SETMASK, &ss_old, NULL)) {
		perror("sigprocmask SIG_SETMASK");
		goto fail;
	}

	close(self[0]);
	close(self[1]);

	return r;

fail:

	/*
	 * Failures which would leave the process in an altered state;
	 * I don't see how we could expect to recover here.
	 */

	abort();
}

