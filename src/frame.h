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

	struct frame *parent;
};

struct frame *
frame_push(struct frame **f);

struct frame *
frame_pop(struct frame **f);

struct var *
frame_set(struct frame *f, size_t n, const char *name,
	struct code *code);

struct var *
frame_get(const struct frame *f, size_t n, const char *name);

#endif

