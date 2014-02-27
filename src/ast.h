#ifndef AST_H
#define AST_H

enum ast_type {
	/* leaves */
	AST_STATUS, /*  $?     */
	AST_STR,    /*  'xyz'  */
	AST_LIST,   /* (x y z) */
	AST_EXEC,   /*  x y z  */

	/* block scope */
	AST_BLOCK,  /* { a } */
	AST_CALL,   /*   a() */
	AST_TICK,   /*  `a   */
	AST_DEREF,  /*  $a   */
	AST_SETBG,  /*   a & */

	/* binary operators */
	AST_AND,    /* a && b */
	AST_OR,     /* a || b */
	AST_JOIN,   /* a  ^ b */
	AST_PIPE,   /* a  | b */
	AST_ASSIGN, /* a  = b */
	AST_SEP     /* a  ; b */
};

struct ast_list {
	struct ast *a;
	struct ast_list *next;
};

struct ast {
	enum ast_type type;

	union {
		char *s;
		int r;

		struct ast_list *l;

		struct {
			struct scope *sc;
			struct ast_list *l;
		} exec;

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
ast_new_status(int r);

struct ast *
ast_new_leaf(enum ast_type type, size_t n, const char *s);

struct ast *
ast_new_list(struct ast_list *l);

struct ast *
ast_new_exec(enum ast_type type, struct scope *sc, struct ast_list *l);

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

