#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../list.h"
#include "../exec.h"
#include "../frame.h"

#include "op.h"

int
eval_ast(struct ast *a, struct ast **out);

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

		if (-1 == eval_ast(p->a, &a)) {
			goto error;
		}

		if (a == NULL) {
			/* TODO: maybe a syntax error */
			argv[i] = "";
			continue;
		}

		/* TODO: i don't like this. so many types here! */
		/* TODO: evaulate a block here; this is the function call mechanism */
		/* TODO: this means we deal with all types; merge it to the switch in eval_ast() below
		 * so - this loop (trying to build an argv) should be the main entry point */
		/* TODO: idea: ast_eval() should output depth-first to a list. make_argv and exec() it when complete */

		switch (a->type) {
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

int
eval_exec(struct frame *f, struct ast_list *l)
{
	char **argv;
	int argc;
	int r;

	assert(f != NULL);

	argv = make_argv(l, &argc);
	if (argv == NULL) {
		return -1;
	}

	r = exec_cmd(f, argc, argv);

	free(argv);

	status = r;

	return 0;
}

static int
eval_op(struct ast *a,
	int (*op)(struct ast *a, struct ast *b))
{
	int r;

	r = op(a->u.op.a, a->u.op.b);
	if (r == -1) {
		return -1;
	}

	status = r;

	return r;
}

int
eval_ast(struct ast *a, struct ast **out)
{
	assert(out != NULL);

	*out = NULL;

	if (a == NULL) {
		return 0;
	}

	switch (a->type) {
	/* identities */
	case AST_STR:
	case AST_LIST:
		*out = a;
		return 0;

	case AST_VAR:
		*out = frame_get(a->f, a->u.s);
		return 0;

	case AST_EXEC:
		return eval_exec(a->f, a->u.l);

	case AST_BLOCK:
		return eval_ast(a->u.a, out);

	case AST_CALL:
		/* TODO: !call hook here */
	case AST_TICK:
	case AST_SETBG:
		/* TODO */
		errno = ENOSYS;
		return -1;

	case AST_ASSIGN:
		assert(a->u.a->type == AST_STR);
		if (!frame_set(a->f, a->u.op.a->u.s, a->u.op.b)) {
			return -1;
		}
		return 0;

	/* binary operators */
	case AST_AND:    return eval_op(a, eval_op_and);
	case AST_OR:     return eval_op(a, eval_op_or);
	case AST_JOIN:   return eval_op(a, eval_op_join);
	case AST_PIPE:   return eval_op(a, eval_op_pipe);
	case AST_SEP:    return eval_op(a, eval_op_sep);

	default:
		errno = EINVAL;
		return -1;
	}
}

int
out_eval(FILE *f, struct ast *a)
{
	struct ast *out;

	assert(f != NULL);
	assert(a != NULL);

	/* TODO: attach to stdout for eval */
	(void) f;

	if (-1 == eval_ast(a, &out)) {
		return -1;
	}

	if (!frame_set(a->f, "_", out)) {
		return -1;
	}

/* XXX:
	ast_free(out);
*/

	return 0;
}

