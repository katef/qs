/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PAIR_H
#define PAIR_H

struct pair {
	int m;
	int n;

	struct pair *next;
};

int
pair_fd(const char *s, int *fd);

struct pair *
pair_push(struct pair **head, int m, int n);

void
pair_free(struct pair *p);

#endif

