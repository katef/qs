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

