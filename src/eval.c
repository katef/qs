#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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

	if (!data_push(data, 0, NULL)) {
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

	status = !status;

	return 0;
}

/* pop variable name; wind from variable to *data and &node->next */
static int
eval_call(struct code *node, struct data **data)
{
	const struct var *q;
	struct code *code_new, **code_tail;
	struct data *data_new, **data_tail;
	struct data *a;

	assert(data != NULL);
	assert(node != NULL);
	assert(node->type == CODE_CALL);
	assert(node->frame != NULL);

	if (*data == NULL) {
		errno = 0;
		return -1;
	}

	a = *data;

	q = frame_get(node->frame, a->s);
	if (q == NULL) {
		fprintf(stderr, "no such variable: $%s\n", a->s);
		errno = 0;
		return -1;
	}

	code_new  = NULL;
	data_new  = NULL;

	code_tail = code_clone(&code_new, q->code);
	data_tail = data_clone(&data_new, q->data);
	if (code_tail == NULL || data_tail == NULL) {
		code_free(code_new);
		data_free(data_new);
		return -1;
	}

	*data = a->next;
	free(a);

	*code_tail = node->next;
	node->next = code_new;

	*data_tail = *data;
	*data      = data_new;

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

	argv = make_argv(*data, &argc);
	if (argv == NULL) {
		return -1;
	}

	status = exec_cmd(node->frame, argc, argv);

	free(argv);

	/* TODO: maybe have make_argv output p, cut off the arg list, and data_free() it */
	for (p = *data; p->s != NULL; p = next) {
		assert(p != NULL);

		free(p);

		next = p->next;
	}

	*data = p;

	return 0;
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

	if (*data == NULL || (*data)->next == NULL) {
		errno = 0;
		return -1;
	}

	a =  *data;
	b = (*data)->next;

	if (-1 == op(&b->next, node->frame, a, b)) {
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

	while (node = *code, node != NULL) {
		switch (node->type) {
		case CODE_NULL: r = eval_null(node, data); break;
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

	return 0;
}

int
eval_clone(const struct code *code, const struct data *data,
	struct data **out)
{
	struct code *code_new, **code_tail;
	struct data *data_new, **data_tail;

	assert(out != NULL);

	code_new  = NULL;
	data_new  = NULL;

	code_tail = code_clone(&code_new, code);
	data_tail = data_clone(&data_new, data);
	if (code_tail == NULL || data_tail == NULL) {
		goto error;
	}

	if (-1 == eval(&code_new, &data_new)) {
		goto error;
	}

	assert(code_new == NULL);

	*out = data_new;

	return 0;

error:

	code_free(code_new);
	data_free(data_new);

	return -1;
}

