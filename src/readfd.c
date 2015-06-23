#define _POSIX_SOURCE

#include <sys/types.h>

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "readfd.h"

#ifdef TEST
#undef PIPE_BUF
#define PIPE_BUF 4
#endif

int
readfd(int fd, char **s, size_t *n)
{
	ssize_t r;
	size_t l;

	assert(fd != -1);
	assert(s != NULL);
	assert(n != NULL);

	if (*s == NULL) {
		*n = 0;
		l = PIPE_BUF;
	} else {
		l = *n;
	}

	do {
		if (*n % PIPE_BUF == 0) {
			char *tmp;

			if (l + PIPE_BUF <= l) {
				errno = ENOMEM;
				goto error;
			}

			l += PIPE_BUF;

			tmp = realloc(*s, l);
			if (tmp == NULL) {
				goto error;
			}

			*s = tmp;
		}

		r = read(fd, *s + *n, l - *n);
		if (r == -1) {
			goto error;
		}

		*n += r;
	} while (r != 0);

	{
		char *tmp;

		tmp = realloc(*s, *n + 1);
		if (tmp == NULL) {
			goto error;
		}

		*s = tmp;

		*n += 1;
	}

	(*s)[*n] = '\0';

	return 0;

error:

	if (errno != EINTR) {
		int e;

		e = errno;
		free(*s);
		errno = e;

		*s = NULL;
	}

	return -1;
}

#ifdef TEST

#include <signal.h>

static void h(int s) {
	fprintf(stderr, "signal %d\n", s);
}

int main(void) {
	size_t n;
	char *s;
	int r;

	s = NULL;

	fprintf(stderr, "%d\n", (int) getpid());

	if (SIG_ERR == signal(SIGUSR1, h)) {
		perror("signal");
		return 1;
	}

	do {
		r = readfd(STDIN_FILENO, &s, &n);

		if (r == -1 && errno == EINTR) {
			fprintf(stderr, "EINTR, s='%s'\n", s);
			continue;
		}

		if (r == -1) {
			perror("readfd");
			return 1;
		}
	} while (r != 0);

	/* here i would tokenise by delimeters (including '\0') to produce a set of strings */

	printf("r=%d, n=%d, s='%s'\n", r, (int) n, s);

	free(s);

	return 0;
}

#endif

