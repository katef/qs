#ifndef AST_H
#define AST_H

enum ast_node_type {
	AST_EXEC,
	AST_NODE
};

struct ast_list {
	char *s;
	struct ast_list *next;
};

struct ast_exec {
	struct ast_list *list;
	/* TODO: flags here (e.g. & for bg) */
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

struct ast_list *
ast_new_list(size_t n, const char *s);

struct ast_exec *
ast_new_exec(struct ast_list *list);

struct ast_node *
ast_new_node_exec(struct ast_exec *exec);

struct ast_node *
ast_new_node_node(struct ast_node *node);

void
ast_free_list(struct ast_list *list);

void
ast_free_exec(struct ast_exec *exec);

void
ast_free_node(struct ast_node *node);

int
ast_dump(const struct ast_node *node);

#endif

