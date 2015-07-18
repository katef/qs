#ifndef CODE_H
#define CODE_H

enum {
	CODE_NONE = (1 << 7)
};

enum code_type {
	CODE_CALL = 0 | CODE_NONE,
	CODE_JOIN = 1 | CODE_NONE,
	CODE_NOT  = 2 | CODE_NONE,
	CODE_NULL = 3 | CODE_NONE,
	CODE_RUN  = 4 | CODE_NONE,
	CODE_TICK = 5 | CODE_NONE,
	CODE_DUP  = 7 | CODE_NONE,
	CODE_PUSH = 8 | CODE_NONE,
	CODE_POP  = 9 | CODE_NONE,

	CODE_DATA = 0, /* u.s */
	CODE_IF   = 1, /* u.code */
	CODE_PIPE = 2, /* u.code */
	CODE_SET  = 3  /* u.code */
};

struct code {
	enum code_type type;

	union {
		char *s;
		struct code *code;
	} u;

	struct code *next;
};

struct frame;

const char *
code_name(enum code_type type);

struct code *
code_anon(struct code **head,
	enum code_type type, struct code *code);

struct code *
code_data(struct code **head,
	size_t n, const char *s);

struct code *
code_push(struct code **head,
	enum code_type type);

int
code_wind(struct code **head, const struct code *code);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

int
code_dump(FILE *f, const struct code *code);

#endif

