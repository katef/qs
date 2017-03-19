/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ARGS_H
#define ARGS_H

struct data;

int
count_args(const struct data *data);

char **
make_args(const struct data *data, int n);

int
set_args(const struct lex_mark *mark, struct frame *frame, char *args[]);

void
dump_args(const char *name, char *const args[]);

#endif

