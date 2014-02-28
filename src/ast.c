#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"
#include "var.h"
#include "frame.h"

struct ast *
ast_new_leaf(enum ast_type type, size_t n, const char *s)
{
	struct ast *new;

	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->f      = NULL;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	return new;
}

struct ast *
ast_new_list(struct ast_list *l)
{
	struct ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type = AST_LIST;
	new->f    = NULL;
	new->u.l  = l;

	return new;
}

struct ast *
ast_new_exec(struct frame *f, enum ast_type type, struct ast_list *l)
{
	struct ast *new;

	assert(f != NULL);

	if (l == NULL) {
		errno = EINVAL;
		return NULL;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type = type;
	new->f    = f;
	new->u.l  = l;

	return new;
}

struct ast *
ast_new_block(struct frame *f, enum ast_type type, struct ast *a)
{
	struct ast *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type  = type;
	new->f     = f;
	new->u.a   = a;

	return new;
}

struct ast *
ast_new_op(struct frame *f, enum ast_type type, struct ast *a, struct ast *b)
{
	struct ast *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->f      = f;
	new->u.op.a = a;
	new->u.op.b = b;

	return new;
}

void
ast_free(struct ast *a)
{
	if (a == NULL) {
		return;
	}

	switch (a->type) {
		struct frame *f;

	case AST_STR:
		break;

	case AST_BLOCK:
	case AST_DEREF:
	case AST_CALL:
		f = frame_pop(&a->f);
		var_free(f->var);
		free(f);
		ast_free(a->u.a);
		break;

	default:
		ast_free(a->u.op.a);
		ast_free(a->u.op.b);
		break;
	}

	free(a);
}

