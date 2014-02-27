#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "ast.h"
#include "list.h"
#include "eval.h"
#include "exec.h"
#include "scope.h"
#include "op.h"

static char **
make_argv(const struct ast_list *l, int *argc)
{
	const struct ast_list *p;
	char **argv;
	int i;

	assert(argc != NULL);
	assert(l != NULL);

	if (l == NULL) {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0, p = l; p != NULL; p = p->next, i++)
		;

	argv = malloc((i + 1) * sizeof *argv);
	if (argv == NULL) {
		return NULL;
	}

	for (i = 0, p = l; p != NULL; p = p->next, i++) {
		struct ast *a;

		a = eval_ast(p->a);
		if (a == NULL) {
			goto error;
		}

		/* TODO: i don't like this. so many types here! */
		switch (a->type) {
		case AST_STATUS:
			/* TODO: maybe a syntax error */
			argv[i] = "";
			break;

		case AST_STR:
			argv[i] = a->u.s;
			break;

		case AST_LIST:
			/* TODO: splice in AST_LIST by recursion */
			errno = ENOSYS;
			goto error;

		default:
			errno = EINVAL;
			goto error;
		}
	}

	*argc = i;
	argv[i] = NULL;

	return argv;

error:

	free(argv);

	return NULL;
}

struct ast *
eval_exec(struct scope *sc, struct ast_list *l)
{
	char **argv;
	int argc;
	int r;
	int n;

	assert(sc != NULL);

	argv = make_argv(l, &argc);
	if (argv == NULL) {
		return NULL;
	}

	assert(l != NULL);

	r = exec_cmd(argc, argv);

	free(argv);

	if (-1 == status_set(sc, r)) {
		return NULL;
	}

	return ast_new_status(r);
}

static struct ast *
eval_op(struct ast *a,
	int (*op)(struct ast *a, struct ast *b))
{
	int r;
	int n;

	r = op(a->u.op.a, a->u.op.b);
	/* XXX: errors? */

	if (-1 == status_set(a->sc, r)) {
		return NULL;
	}

	return ast_new_status(r);
}

struct ast *
eval_ast(struct ast *a)
{
	while (a != NULL) {
		switch (a->type) {

		/* identities */
		case AST_STATUS:
		case AST_STR:
		case AST_LIST:
			return a;

		case AST_EXEC:
			a = eval_exec(a->sc, a->u.l);
			continue;

		case AST_BLOCK:
			a = a->u.a;
			/* TODO: DEBUG_ to dump symbol table after executing a block */
			continue;

		case AST_CALL:
		case AST_TICK:
		case AST_DEREF:
		case AST_SETBG:
			if (!scope_set(a->sc, "?", "1")) {
				return NULL;
			}

			/* TODO */
			errno = ENOSYS;
			return NULL;

		/* binary operators */
		case AST_AND:    a = eval_op(a, op_and);    continue;
		case AST_OR:     a = eval_op(a, op_or);     continue;
		case AST_JOIN:   a = eval_op(a, op_join);   continue;
		case AST_PIPE:   a = eval_op(a, op_pipe);   continue;
		case AST_ASSIGN: a = eval_op(a, op_assign); continue;
		case AST_SEP:    a = eval_op(a, op_sep);    continue;

		default:
			errno = EINVAL;
			return NULL;
		}
	}

	return NULL;
}

