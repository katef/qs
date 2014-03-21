#ifndef CODE_H
#define CODE_H

enum code_type {
	/* terminals */
	CODE_NULL = 1 <<  0,
	CODE_ANON = 1 <<  1,
	CODE_RET  = 1 <<  2,
	CODE_DATA = 1 <<  3,
	CODE_NOT  = 1 <<  4,

	/* operators */
	CODE_TICK = 1 <<  5,
	CODE_CALL = 1 <<  6,
	CODE_EXEC = 1 <<  7,
	CODE_IF   = 1 <<  8,
	CODE_JOIN = 1 <<  9,
	CODE_PIPE = 1 << 10,
	CODE_SET  = 1 << 11
};

struct code {
	enum code_type type;
	struct frame *frame;

	union {
		char *s;
		struct code *code;
	} u;

	struct code *next;
};

const char *
code_name(enum code_type type);

struct code *
code_anon(struct code **head, struct frame *frame,
	struct code *code);

struct code *
code_data(struct code **head, struct frame *frame,
	size_t n, const char *s);

struct code *
code_push(struct code **head, struct frame *frame,
	enum code_type type);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

struct code **
code_clone(struct code **dst, const struct code *src);

int
code_dump(FILE *f, const struct code *code);

#endif

