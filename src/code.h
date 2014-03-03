#ifndef CODE_H
#define CODE_H

enum code_type {
	/* terminals */
	CODE_NULL = 1 << 0, /* none   */
	CODE_NOT  = 1 << 1, /* none   */
	CODE_DATA = 1 << 2, /* u.data */
	CODE_CODE = 1 << 3, /* u.code */

	/* operators (u.frame) */
	CODE_CALL = 1 << 4,
	CODE_EXEC = 1 << 5,
	CODE_IF   = 1 << 6,
	CODE_JOIN = 1 << 7,
	CODE_PIPE = 1 << 8,
	CODE_BIND = 1 << 9
};

struct code {
	enum code_type type;

	union {
		struct data  *data;
		struct code  *code;
		struct frame *frame;
	} u;

	struct code *next;
};

struct code *
code_push(struct code **head, enum code_type type, void *p);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

struct code **
code_clone(struct code **dst, const struct code *src);

#endif

