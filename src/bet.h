#ifndef BET_H
#define BET_H

enum bet_type {
	/* leaves */
	BET_STR,    /* 'xyz' */
	BET_VAR,    /* $xyz  */

	/* branches */
	BET_AND,    /* a && b */
	BET_OR,     /* a || b */
	BET_JOIN,   /* a  ^ b */
	BET_PIPE,   /* a  | b */
	BET_ASSIGN, /* a  = b */
	BET_EXEC,   /* a  ; b */
	BET_BG,     /* a  & b */
	BET_LIST    /* a    b */
};

struct bet {
	enum bet_type type;

	union {
		char *s;

		struct {
			struct bet *a;
			struct bet *b;
		} op;
	} u;
};

struct bet *
bet_new_leaf(enum bet_type type, size_t n, const char *s);

struct bet *
bet_new_branch(enum bet_type type, struct bet *a, struct bet *b);

void
bet_free(struct bet *bet);

int
bet_dump(const struct bet *bet);

#endif

