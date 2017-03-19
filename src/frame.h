/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FRAME_H
#define FRAME_H

struct var;
struct pair;
struct code;

struct frame {
	struct var *var;

	/*
	 * Lists of fd pairs.
	 *
	 * dup: [m=n] to dup2()
	 *   .m is oldfd. .m = -1 means to close .n, rather than dup2 over .n
	 *   .n is newfd. never -1
	 *
	 * asc: [m|n] to dup() over pipe endpoints
	 *   .m is fd[1] of the lhs
	 *   .n is fd[0] of the rhs
	 */
	struct pair *dup;
	struct pair *asc; 

	/*
	 * Tasks branch and share their stack of frames up to that point.
	 * Each task may then push its own new frames.
	 *
	 * In general we do not know which task will exit first, and so
	 * reference counting is used here to know when to destroy the frames
	 * which are shared between both tasks, up to the branch point.
	 */
	unsigned int refcount;

	struct frame *parent;
};

struct frame *
frame_push(struct frame **f);

struct frame *
frame_pop(struct frame **f);

void
frame_free(struct frame *f);

int
frame_refcount(struct frame *f, int delta);

struct var *
frame_set(struct frame *f, size_t n, const char *name,
	struct code *code);

struct var *
frame_get(const struct frame *f, size_t n, const char *name);

#endif

