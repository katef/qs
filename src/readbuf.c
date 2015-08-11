#define _POSIX_SOURCE

#include <sys/types.h>

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "readbuf.h"

int
readbuf(int fd, char **s, size_t *n)
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
	if (r == -1 && errno == EINTR) {
		return -1;
	}

	if (r == -1) {
		goto error;
	}

	if (r == 0) {
		char *tmp;

		tmp = realloc(*s, *n + 1);
		if (tmp == NULL) {
			goto error;
		}

		*s = tmp;

		*n += 1;

		(*s)[*n] = '\0';

		return 0;
	}

	*n += r;

	return (int) r;

error:

	{
		int e;

		e = errno;
		free(*s);
		errno = e;

		*s = NULL;
	}

	return -1;
}

