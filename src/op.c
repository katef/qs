#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "eval.h"
#include "ast.h"
#include "op.h"
#include "status.h"

int
op_and(struct ast *a, struct ast *b)
{
	struct ast *q;
	int r;
/* TODO: return -1 for error, 0 for success. use $? seperately */

	/* XXX: i don't like this at all */
	if (-1 == eval_ast(a, &q)) {
		return -1;
	}

	if (q != NULL) {
		errno = EINVAL;
		return -1;
	}

	if (-1 == status_get(a->sc, &r)) {
		return -1;
	}

	if (r != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (-1 == eval_ast(b, &q)) {
		return -1;
	}

	if (q != NULL) {
		errno = EINVAL;
		return -1;
	}

	if (-1 == status_get(b->sc, &r)) {
		return -1;
	}

	if (r != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int
op_or(struct ast *a, struct ast *b)
{
	struct ast *q;
	int r;

	/* XXX: i don't like this at all */
	if (-1 == eval_ast(a, &q)) {
		return -1;
	}

	if (q != NULL) {
		errno = EINVAL;
		return -1;
	}

	if (-1 == status_get(a->sc, &r)) {
		return -1;
	}

	if (r == EXIT_SUCCESS) {
		return EXIT_SUCCESS;
	}

	if (-1 == eval_ast(b, &q)) {
		return -1;
	}

	if (q != NULL) {
		errno = EINVAL;
		return -1;
	}

	if (-1 == status_get(b->sc, &r)) {
		return -1;
	}

	if (r == EXIT_SUCCESS) {
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

int
op_join(struct ast *a, struct ast *b)
{
	/* TODO: map down based on $FS (field seperator) */
	(void) a;
	(void) b;

	return -1;
}

int
op_pipe(struct ast *a, struct ast *b)
{
	/* TODO */
	(void) a;
	(void) b;

	errno = ENOSYS;
	return -1;
}

int
op_assign(struct ast *a, struct ast *b)
{
	/* TODO */
	(void) a;
	(void) b;

	errno = ENOSYS;
	return -1;
}

int
op_sep(struct ast *a, struct ast *b)
{
	struct ast *q;

	if (a != NULL) {
		if (-1 == eval_ast(a, &q)) {
			return -1;
		}

		if (q != NULL) {
			errno = EINVAL;
			return -1;
		}
	}

	if (b != NULL) {
		if (-1 == eval_ast(b, &q)) {
			return -1;
		}

		if (q != NULL) {
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

