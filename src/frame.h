#ifndef FRAME_H
#define FRAME_H

struct var;
struct pair;
struct code;

struct frame {
	struct var *var;

	/* .m is oldfd; .n is newfd.
	 * .m = -1 means to close .n, rather than dup2 over .n; .n is never -1 */
	struct pair *dup;

	/* .m is fd[0]; .n is fd[1] */
	struct pair *pipe;

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

