#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <glob.h>

static int
globerr(const char *path, int e)
{
	assert(path != NULL);

	fprintf(stderr, "%s: %s\n", path, strerror(e));
	exit(1);
}

int
main(int argc, char *argv[])
{
	int i, j;

	for (i = 1; i < argc; i++) {
		glob_t g;
		int r;

		r = glob(argv[i], GLOB_ERR | GLOB_NOSORT, globerr, &g);
		switch (r) {
		case GLOB_NOMATCH:
			puts(argv[i]);
			continue;

		case 0:
			break;

		case GLOB_NOSPACE: errno = ENOMEM; goto error;
		case GLOB_ABORTED: errno = EAGAIN; goto error;
		default:           errno = EINVAL; goto error;
		}

		for (j = 0; j < g.gl_pathc; j++) {
			puts(g.gl_pathv[j]);
		}

		globfree(&g);
	}

	return 0;

error:

	perror("glob");

	return 1;
}

