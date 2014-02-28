#ifndef FRAME_H
#define FRAME_H

struct ast;

struct frame {
	struct var *var;
	struct frame *parent;
};

extern int status;

struct var **
frame_push(struct frame **f);

struct frame *
frame_pop(struct frame **f);

struct var *
frame_set(struct frame *f, const char *name, struct ast *val);

struct ast *
frame_get(const struct frame *f, const char *name);

#endif

