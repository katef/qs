/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <sys/types.h>

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	pid_t r;

	argv++;

	if (argv[0] == NULL) {
		fprintf(stderr, "usage: bg cmd [args...]\n");
		return 1;
	}

	r = fork();
	if (r == -1) {
		perror("fork");
		return 1;
	}

	if (r != 0) {
		return 0;
	}

	/* TODO: want qs's exec hooks here */

	if (-1 == execvp(argv[0], argv)) {
		perror(argv[0]);
		return 1;
	}

	/* NOTREACHED */

	abort();
}

