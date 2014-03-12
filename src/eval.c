#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "var.h"
#include "data.h"
#include "code.h"
#include "exec.h"
#include "eval.h"
#include "frame.h"

/* push .s=NULL to data */
static int
eval_null(struct code *node, struct data **data)
{
	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_NULL);

	(void) node;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_null\n");
	}

	if (!data_push(data, 0, NULL)) {
		return -1;
	}

	return 0;
}

static int
eval_anon(struct code *node, struct data **data)
{
	struct code *a;

	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_ANON);

	(void) data;

	a = node;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s\n", code_name(a->type));
	}

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "clone: ");
		code_dump(stderr, node->u.code);
	}

	if (!code_clone(&node->next, node->u.code)) {
		return -1;
	}

	return 0;
}

/* push .s=NULL to data */
static int
eval_data(struct code *node, struct data **data)
{
	struct code *a;

	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_DATA);

	a = node;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s \"%s\"\n", code_name(a->type), a->u.s);
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_data: \"%s\"\n", a->u.s);
	}

	if (!data_push(data, strlen(a->u.s), a->u.s)) {
		return -1;
	}

	return 0;
}

/* pop $?, push !$? to *data */
static int
eval_not(struct code *node, struct data **data)
{
	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_NOT);

	(void) node;
	(void) data;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_not\n");
	}

	status = !status;

	return 0;
}

/* pop variable name; wind from variable to *data and &node->next */
static int
eval_call(struct code *node, struct data **data)
{
	const struct var *q;
	struct data *a;

	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_CALL);
	assert(node->u.frame != NULL);

	if (*data == NULL) {
		errno = 0;
		return -1;
	}

	a = *data;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data <- %s\n", a->s ? a->s : "NULL");
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_call: \"%s\"\n", a->s);
	}

	q = frame_get(node->u.frame, strlen(a->s), a->s);
	if (q == NULL) {
		fprintf(stderr, "no such variable: $%s\n", a->s);
		errno = 0;
		return -1;
	}

	if (!code_anon(&node->next, q->code)) {
		return -1;
	}

	*data = a->next;
	free(a);

	return 0;
}

/* eat whole data stack upto .s=NULL, make argv and execute */
static int
eval_exec(struct code *node, struct data **data)
{
	struct data *p, *next;
	char **argv;
	int argc;

	assert(node != NULL);
	assert(data != NULL);
	assert(node->type == CODE_EXEC);

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_exec\n");
	}

	if (debug & DEBUG_STACK) {
		for (p = *data; p->s != NULL; p = p->next) {
			fprintf(stderr, "data <- %s\n", p->s ? p->s : "NULL");
		}
	}

	argc = count_args(*data);

	if (argc == 0) {
		goto done;
	}

	argv = make_args(*data, argc + 1);
	if (argv == NULL) {
		return -1;
	}

	errno = 0;

	status = exec_cmd(node->u.frame, argc, argv);

	if (status == -1 && errno != 0) {
		goto error;
	}

done:

	/* TODO: maybe have make_args output p, cut off the arg list, and data_free() it */
	for (p = *data; p->s != NULL; p = next) {
		assert(p != NULL);

		next = p->next;
		free(p);
	}

	*data = p->next;
	free(p);

	return 0;

error:

	perror(argv[0]);

	free(argv);

	return -1;
}

/* TODO: explain what happens here: status is the predicate, CODE_ANON is a block to call */
static int
eval_if(struct code *node, struct data **data)
{
	struct code *a;

	assert(node != NULL);
	assert(data != NULL);
	assert(node->type == CODE_IF);

	if (node->next == NULL) {
		errno = 0;
		return -1;
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_if\n");
	}

	a = node->next;
	if (a->type != CODE_ANON) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * When the status condition is true, we simply leave the forthcoming node
	 * on the code stack to evaulate next. Otherwise, it is consumed and
	 * discarded here.
	 */

	if (status != EXIT_SUCCESS) {
		if (debug & DEBUG_STACK) {
			fprintf(stderr, "code <- %s %s\n", code_name(a->type),
				a->type == CODE_DATA ? a->u.s : "");
		}

		node->next = a->next;
		free(a);
	}

	return 0;
}

/* TODO: explain what happens here */
static int
eval_set(struct code *node, struct data **data)
{
	struct data *a;
	struct code *b;

	assert(node != NULL);
	assert(data != NULL);
	assert(node->type == CODE_SET);

	(void) node;
	(void) a;
	(void) b;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_set\n");
	}

	if (node->next == NULL || *data == NULL) {
		errno = 0;
		return -1;
	}

	a = *data;
	b = node->next;

	if (b->type != CODE_ANON) {
		errno = 0;
		return -1;
	}

	if (!frame_set(node->u.frame, strlen(a->s), a->s, b->u.code)) {
		errno = EINVAL;
		return -1;
	}

	*data = a->next;
	free(a);

	node->next = b->next;
	free(b);

	return 0;
}

/* TODO: explain what happens here */
static int
op_join(struct data **node, struct frame *frame, struct data *a, struct data *b)
{
	(void) node;
	(void) frame;
	(void) a;
	(void) b;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_join\n");
	}

	/* TODO */
	errno = ENOSYS;
	return -1;
}

/* TODO: explain what happens here */
static int
op_pipe(struct data **node, struct frame *frame, struct data *a, struct data *b)
{
	(void) node;
	(void) frame;
	(void) a;
	(void) b;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_pipe\n");
	}

	/* TODO */
	errno = ENOSYS;
	return -1;
}

static int
eval_binop(struct code *node, struct data **data,
	int (*op)(struct data **, struct frame *, struct data *a, struct data *b))
{
	struct data *a;
	struct data *b;

	assert(node != NULL);
	assert(data != NULL);
	assert(op != NULL);

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_binop\n");
	}

	if (*data == NULL || (*data)->next == NULL) {
		errno = 0;
		return -1;
	}

	a =  *data;
	b = (*data)->next;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data <- %s\n", a->s);
		fprintf(stderr, "data <- %s\n", b->s);
	}

	if (-1 == op(&b->next, node->u.frame, a, b)) {
		return -1;
	}

	*data = b->next;
	free(a);
	free(b);

	return 0;
}

static int
eval(struct code **code, struct data **data)
{
	struct code *node;
	int r;

	assert(code != NULL);
	assert(data != NULL);

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval code: ");
		code_dump(stderr, *code);
	}

	while (node = *code, node != NULL) {
		switch (node->type) {
		case CODE_NULL: r = eval_null(node, data); break;
		case CODE_ANON: r = eval_anon(node, data); break;
		case CODE_DATA: r = eval_data(node, data); break;
		case CODE_NOT:  r = eval_not (node, data); break;
		case CODE_IF:   r = eval_if  (node, data); break;
		case CODE_CALL: r = eval_call(node, data); break;
		case CODE_EXEC: r = eval_exec(node, data); break;
		case CODE_SET:  r = eval_set (node, data); break;

		case CODE_JOIN: r = eval_binop(node, data, op_join); break;
		case CODE_PIPE: r = eval_binop(node, data, op_pipe); break;

		default:
			errno = EINVAL;
			return -1;
		}

		if (r == -1) {
			return -1;
		}

		*code = node->next;
		free(node);
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval out: ");
		data_dump(stderr, *data);
	}

	return 0;
}

/* XXX: get rid of this; <dispatch> can modify @c, @d */
int
eval_clone(const struct code *code, struct data **out)
{
	struct code *code_new;

	assert(out != NULL);

	code_new  = NULL;
	*out      = NULL;

/* TODO: could DRY by pushing CODE_ANON here instead */
	if (!code_clone(&code_new, code)) {
		goto error;
	}

	if (-1 == eval(&code_new, out)) {
		goto error;
	}

	assert(code_new == NULL);

	return 0;

error:

	code_free(code_new);
	data_free(*out);

	return -1;
}

