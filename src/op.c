#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "eval.h"
#include "ast.h"
#include "op.h"

int
op_and(struct ast *a, struct ast *b)
{
	/* XXX: i don't like this at all */
	a = eval_ast(a);
	if (a->type != AST_STATUS) {
		errno = EINVAL;
		return -1;
	}

	if (0 != a->u.r) {
		return EXIT_FAILURE;
	}

	b = eval_ast(b);
	if (b->type != AST_STATUS) {
		errno = EINVAL;
		return -1;
	}

	if (0 != b->u.r) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int
op_or(struct ast *a, struct ast *b)
{
	/* XXX: i don't like this at all */
	a = eval_ast(a);
	if (a->type != AST_STATUS) {
		errno = EINVAL;
		return -1;
	}

	if (0 == a->u.r) {
		return EXIT_SUCCESS;
	}

	b = eval_ast(b);
	if (b->type != AST_STATUS) {
		errno = EINVAL;
		return -1;
	}

	if (0 == b->u.r) {
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

int
op_join(struct ast *a, struct ast *b)
{
	/* TODO: map down based on $FS (field seperator) */
	(void) a, b;

	return -1;
}

int
op_pipe(struct ast *a, struct ast *b)
{
	/* TODO */
	(void) a, b;

	errno = ENOSYS;
	return -1;
}

int
op_assign(struct ast *a, struct ast *b)
{
	/* TODO */
	(void) a, b;

	errno = ENOSYS;
	return -1;
}

int
op_sep(struct ast *a, struct ast *b)
{
	struct ast *r;

	r = NULL;

	if (a != NULL) {
		a = eval_ast(a);
		if (a->type != AST_STATUS) {
			errno = EINVAL;
			return -1;
		}

		r = a;
	}

	if (b != NULL) {
		b = eval_ast(b);
		if (b->type != AST_STATUS) {
			errno = EINVAL;
			return -1;
		}

		r = b;
	}

	if (r == NULL) {
		return EXIT_SUCCESS;
	}

	return r->u.r;
}

