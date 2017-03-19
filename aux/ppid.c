/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

int main(void) {
	pid_t pid;

	pid = getppid();
	(void) printf("%ld\n", (long) pid);

	return 0;
}

