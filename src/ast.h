#ifndef AST_H
#define AST_H

enum ast_node_type {
	AST_EXEC,
	AST_NODE
};

struct ast_arg {
	char *s;
	struct ast_arg *next;
};

struct ast_exec {
	char *s;
	struct ast_arg *arg;
};

struct ast_node {
	enum ast_node_type type;
	union {
		struct ast_exec *exec;
		struct ast_node *node; /* conceptually equivalent to a block */
		/* TODO: other command types: functions, conditionals, assignments etc here */
	} u;

	/* TODO: locally-scoped variables here */

	struct ast_node *next;
};

struct ast_arg *
ast_new_arg(size_t n, const char *s);

struct ast_exec *
ast_new_exec(size_t n, const char *s);

struct ast_node *
ast_new_node_exec(struct ast_exec *exec);

struct ast_node *
ast_new_node_node(struct ast_node *node);

void
ast_free_arg(struct ast_arg *arg);

void
ast_free_exec(struct ast_exec *exec);

void
ast_free_node(struct ast_node *node);

int
ast_dump(const struct ast_node *node);

#endif

