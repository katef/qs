#ifndef STACK_H
#define STACK_H

enum stack_type {
	/* terminals */
	STACK_STR    = 1 << 0,

	/* operators */
	STACK_DEREF  = 1 << 2,
	STACK_AND    = 1 << 3,
	STACK_OR     = 1 << 4,
	STACK_JOIN   = 1 << 5,
	STACK_PIPE   = 1 << 6,
	STACK_ASSIGN = 1 << 7
};

struct stack {
	enum stack_type type;

	union {
		char *s;
		struct frame *f;
	} u;

	struct stack *next;
};

struct stack *
stack_push_str(struct stack *tail, size_t n, const char *s);

struct stack *
stack_push_op(struct stack *tail, enum stack_type, struct frame *f);

struct stack *
stack_pop(struct stack **head, unsigned int mask);

#endif

