#ifndef OP_H
#define OP_H

enum {
	CODE_NONE = (1 << 7)
};

enum opcode {
	OP_CALL =  0 | CODE_NONE,
	OP_JOIN =  1 | CODE_NONE,
	OP_NOT  =  2 | CODE_NONE,
	OP_NULL =  3 | CODE_NONE,
	OP_RUN  =  4 | CODE_NONE,
	OP_TICK =  5 | CODE_NONE,
	OP_DUP  =  7 | CODE_NONE,
	OP_ASC  =  8 | CODE_NONE,
	OP_PUSH =  9 | CODE_NONE,
	OP_POP  = 10 | CODE_NONE,
	OP_CLHS = 11 | CODE_NONE,
	OP_CRHS = 12 | CODE_NONE,
	OP_CTCK = 13 | CODE_NONE,

	OP_DATA = 0, /* u.s */
	OP_IF   = 1, /* u.code */
	OP_PIPE = 2, /* u.code */
	OP_SET  = 3  /* u.code */
};

struct code {
	struct lex_mark *mark;
	enum opcode op;

	union {
		char *s;
		struct code *code;
	} u;

	struct code *next;
};

struct frame;

const char *
op_name(enum opcode op);

struct code *
code_anon(struct code **head, const struct lex_mark *mark,
	enum opcode op, struct code *code);

struct code *
code_data(struct code **head, const struct lex_mark *mark,
	size_t n, const char *s);

struct code *
code_push(struct code **head, const struct lex_mark *mark,
	enum opcode op);

int
code_wind(struct code **head,
	const struct code *code);

struct code *
code_pop(struct code **head);

void
code_free(struct code *code);

void
code_dump(FILE *f, const struct code *code);

#endif

