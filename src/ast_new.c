#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

struct ast_arg *
ast_new_arg(size_t n, const char *s)
{
	struct ast_arg *arg;

	assert(s != NULL);

	arg = malloc(sizeof *arg + n + 1);
	if (arg == NULL) {
		return NULL;
	}

	arg->s    = (char *) arg + sizeof *arg;
	arg->next = NULL;

	memcpy(arg->s, s, n);
	arg->s[n] = '\0';

	return arg;
}

struct ast_exec *
ast_new_exec(size_t n, const char *s)
{
	struct ast_exec *exec;

	assert(s != NULL);

	exec = malloc(sizeof *exec + n + 1);
	if (exec == NULL) {
		return NULL;
	}

	exec->s   = (char *) exec + sizeof *exec;
	exec->arg = NULL;

	memcpy(exec->s, s, n);
	exec->s[n] = '\0';

	return exec;
}

struct ast_node *
ast_new_node_exec(struct ast_exec *exec)
{
	struct ast_node *node;

	assert(exec != NULL);

	node = malloc(sizeof *node);
	if (node == NULL) {
		return NULL;
	}

	node->type   = AST_EXEC;
	node->u.exec = exec;
	node->next   = NULL;

	return node;
}

struct ast_node *
ast_new_node_node(struct ast_node *child)
{
	struct ast_node *node;

	assert(child != NULL);

	node = malloc(sizeof *node);
	if (node == NULL) {
		return NULL;
	}

	node->type   = AST_NODE;
	node->u.node = child;
	node->next   = NULL;

	return node;
}

