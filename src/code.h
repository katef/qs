#ifndef CODE_H
#define CODE_H

enum code_type {
	/* terminals */
	CODE_NULL = 1 << 0,
	CODE_WIND = 1 << 1,

	/* operators */
	CODE_CALL = 1 << 2,
	CODE_EXEC = 1 << 3,
	CODE_AND  = 1 << 4,
	CODE_OR   = 1 << 5,
	CODE_JOIN = 1 << 6,
	CODE_PIPE = 1 << 7,
	CODE_BIND = 1 << 8
};

struct code {
	enum code_type type;

	union {
		struct data  *data;
		struct frame *frame;
	} u;

	struct code *next;
};

struct code *
code_push(struct code **head, enum code_type type, void *p);

struct code *
code_pop(struct code **head);

#endif

