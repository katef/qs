#ifndef AST_H
#define AST_H

enum ast_type {
	/* leaves */
	AST_STR,    /* 'xyz' */

	/* block scope */
	AST_BLOCK,  /* { a } */
	AST_DEREF,  /*  $a   */
	AST_CALL,   /*   a() */
	AST_RUNFG,  /*   a   */
	AST_RUNBG,  /*   a & */

	/* binary operators */
	AST_AND,    /* a && b */
	AST_OR,     /* a || b */
	AST_JOIN,   /* a  ^ b */
	AST_PIPE,   /* a  | b */
	AST_ASSIGN, /* a  = b */
	AST_SEP,    /* a  ; b */
	AST_CONS    /* a    b */
};

struct ast {
	enum ast_type type;

	union {
		char *s;

		struct {
			struct scope *sc;
			struct ast *a;
		} block;

		struct {
			struct ast *a;
			struct ast *b;
		} op;
	} u;
};

struct ast *
ast_new_leaf(enum ast_type type, size_t n, const char *s);

struct ast *
ast_new_block(enum ast_type type, struct scope *sc, struct ast *a);

struct ast *
ast_new_conv(enum ast_type type, struct ast *a);

struct ast *
ast_new_op(enum ast_type type, struct ast *a, struct ast *b);

void
ast_free(struct ast *bet);

int
ast_dump(const struct ast *bet);

#endif

