/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef DUP_H
#define DUP_H

struct frame;

int
dup_find(const struct frame *frame, int newfd);

int
dup_apply(const struct frame *frame);

void
dup_dump(FILE *f, const struct frame *frame);

#endif

