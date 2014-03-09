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
	assert(node->type == CODE_NULL);

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
	struct code *code_new, **code_tail;
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

	q = frame_get(node->u.frame, a->s);
	if (q == NULL) {
		fprintf(stderr, "no such variable: $%s\n", a->s);
		errno = 0;
		return -1;
	}

	code_new  = NULL;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "clone: ");
		code_dump(stderr, q->code);
	}

	code_tail = code_clone(&code_new, q->code);
	if (code_tail == NULL) {
		code_free(code_new);
		return -1;
	}

	*data = a->next;
	free(a);

	*code_tail = node->next;
	node->next = code_new;

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

		free(p);

		next = p->next;
	}

	*data = p->next;

	return 0;

error:

	perror(argv[0]);

	free(argv);

	return -1;
}

/* TODO: explain what happens here: status is the predicate, a is a variable to call */
static int
eval_if(struct code *node, struct data **data)
{
	struct code *a;
	int r;

	assert(node != NULL);
	assert(data != NULL);
	assert(node->type == CODE_IF);

	if (node->next == NULL || *data == NULL) {
		errno = 0;
		return -1;
	}

	/* TODO: could also check *data is $"" */

	a = node->next;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s %s\n", code_name(a->type),
			a->type == CODE_DATA ? a->u.s : "");
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_if: \"%s\"\n", (*data)->s);
	}

	if (status != EXIT_SUCCESS) {
		return 0;
	}

	if (a->type != CODE_CALL) {
		errno = EINVAL;
		return -1;
	}

	r = eval_call(a, data);

	node->next = a->next;
	free(a);

	return r;
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

/* TODO: explain what happens here */
static int
op_set(struct data **node, struct frame *frame, struct data *a, struct data *b)
{
	(void) node;
	(void) frame;
	(void) a;
	(void) b;

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval_set\n");
	}

	/* TODO: could check *data is $"" */

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
		case CODE_DATA: r = eval_data(node, data); break;
		case CODE_NOT:  r = eval_not (node, data); break;
		case CODE_IF:   r = eval_if  (node, data); break;
		case CODE_CALL: r = eval_call(node, data); break;
		case CODE_EXEC: r = eval_exec(node, data); break;

		case CODE_JOIN: r = eval_binop(node, data, op_join); break;
		case CODE_PIPE: r = eval_binop(node, data, op_pipe); break;
		case CODE_SET:  r = eval_binop(node, data, op_set ); break;

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
	struct code *code_new, **code_tail;

	assert(out != NULL);

	code_new  = NULL;
	*out      = NULL;

	code_tail = code_clone(&code_new, code);
	if (code_tail == NULL) {
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

