#ifndef FRAME_H
#define FRAME_H

struct frame {
	struct var *var;
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

