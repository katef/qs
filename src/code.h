#ifndef CODE_H
#define CODE_H

enum {
	CODE_NONE = (1 << 7)
};

enum code_type {
	CODE_CALL = 0 | CODE_NONE,
	CODE_EXEC = 1 | CODE_NONE,
	CODE_JOIN = 2 | CODE_NONE,
	CODE_NOT  = 3 | CODE_NONE,
	CODE_NULL = 4 | CODE_NONE,
	CODE_RET  = 5 | CODE_NONE,

	CODE_DATA = 0, /* u.s */
	CODE_DUP  = 1, /* u.dup */
	CODE_IF   = 2, /* u.code */
	CODE_PIPE = 3, /* u.code */
	CODE_SET  = 4, /* u.code */
	CODE_TICK = 5  /* u.code */
};

struct code {
	enum code_type type;
	struct frame *frame;

	union {
		char *s;
		struct code *code;
		struct dup  *dup;
	} u;

	struct code *next;
};

const char *
code_name(enum code_type type);

struct code *
code_anon(struct code **head, struct frame *frame,
	enum code_type type, struct code *code);

struct code *
code_data(struct code **head, struct frame *frame,
	size_t n, const char *s);

struct code *
code_dup(struct code **head, struct frame *frame,
	struct dup *dup);

struct code *
code_push(struct code **head, struct frame *frame,
	enum code_type type);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

int
code_dump(FILE *f, const struct code *code);

#endif

