#ifndef CODE_H
#define CODE_H

enum code_type {
	/* terminals */
	CODE_NULL = 1 << 0,
	CODE_DATA = 1 << 1,
	CODE_NOT  = 1 << 2,

	/* operators */
	CODE_CALL = 1 << 3,
	CODE_EXEC = 1 << 4,
	CODE_IF   = 1 << 5,
	CODE_JOIN = 1 << 6,
	CODE_PIPE = 1 << 7,
	CODE_SET  = 1 << 8
};

struct code {
	enum code_type type;

	union {
		char *s;
		struct frame *frame;
	} u;

	struct code *next;
};

struct code *
code_data(struct code **head, size_t n, const char *s);

struct code *
code_push(struct code **head, enum code_type type, struct frame *frame);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

struct code **
code_clone(struct code **dst, const struct code *src);

#endif

