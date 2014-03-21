#define _POSIX_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "debug.h"
#include "var.h"
#include "data.h"
#include "code.h"
#include "args.h"
#include "eval.h"
#include "proc.h"
#include "frame.h"
#include "return.h"
#include "status.h"
#include "builtin.h"

/* push .s=NULL to data */
static int
eval_null(const struct code **next, struct rtrn **rtrn, struct data **data)
{
	assert(next != NULL);
	assert(data != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	(void) next;
	(void) rtrn;

	if (!data_push(data, 0, NULL)) {
		return -1;
	}

	return 0;
}

/* push current code pointer to rtrn stack; set current code pointer to anon block */
static int
eval_anon(const struct code **next, struct rtrn **rtrn, struct data **data,
	const struct code *ci)
{
	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	(void) data;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s\n", code_name(CODE_DATA));
	}

	if (!rtrn_push(rtrn, *next)) {
		return -1;
	}

	*next = ci;

	return 0;
}

/* pop from return stack; set current code pointer to that */
static int
eval_ret(const struct code **next, struct rtrn **rtrn, struct data **data)
{
	struct rtrn *a;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	(void) data;

	a = rtrn_pop(rtrn);
	if (a == NULL) {
		errno = 0;
		return -1;
	}

	*next = a->code;

	free(a);

	return 0;
}

/* push .s=NULL to data */
static int
eval_data(const struct code **next, struct rtrn **rtrn, struct data **data,
	const char *s)
{
	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(s != NULL);

	(void) rtrn;
	(void) next;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s \"%s\"\n", code_name(CODE_DATA), s);
	}

	if (!data_push(data, strlen(s), s)) {
		return -1;
	}

	return 0;
}

/* pop $?, push !$? to *data */
static int
eval_not(const struct code **next, struct rtrn **rtrn, struct data **data)
{
	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	(void) next;
	(void) rtrn;
	(void) data;

	status_exit(!status.r);

	return 0;
}

/* pop variable name; wind from variable to *data and next */
static int
eval_call(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame, const struct code *ci)
{
	const struct var *q;
	struct data *a;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	(void) next;

	if (*data == NULL) {
		errno = 0;
		return -1;
	}

	a = *data;
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data <- %s\n", a->s ? a->s : "NULL");
	}

	q = frame_get(frame, strlen(a->s), a->s);
	if (q == NULL) {
		fprintf(stderr, "no such variable: $%s\n", a->s);
		errno = 0;
		return -1;
	}

	/* XXX: share guts with eval_anon */

	if (!rtrn_push(rtrn, *next)) {
		return -1;
	}

	*next = ci;

	*data = a->next;
	free(a);

	return 0;
}

/* eat whole data stack upto .s=NULL, make argv and execute */
static int
eval_exec(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame, enum code_type type)
{
	struct data *p, *pnext;
	struct var *v;
	pid_t pid;
	int argc;
	char **args;
	FILE *tick;
	int fd[2];

	assert(next != NULL);
	assert(type == CODE_EXEC || type == CODE_TICK);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	(void) next;

	if (debug & DEBUG_STACK) {
		for (p = *data; p->s != NULL; p = p->next) {
			fprintf(stderr, "data <- %s\n", p->s ? p->s : "NULL");
		}
	}

	/* TODO: run pre/post-command hook here (on *data) */

	argc = count_args(*data);
	if (argc == 0) {
		goto done;
	}

	args = make_args(*data, argc + 1);
	if (args == NULL) {
		return -1;
	}

	/* variable */
	v = frame_get(frame, strlen(args[0]), args[0]);
	if (v != NULL) {
		/* XXX: share guts with eval_anon */

		if (!rtrn_push(rtrn, *next)) {
			return -1;
		}

		*next = v->code;

		if (-1 == set_args(frame, args + 1)) {
			free(args);
			return -1;
		}

		goto done;
	}

	/* builtin */
	switch (builtin(frame, argc, args)) {
	case -1: goto error;
	case  0: goto done;
	case  1: break;
	}

	/* TODO: set stdio pipes here, before command execution */
	/* TODO: maybe pass function pointers for before/after pipe stuff */
	if (type == CODE_TICK) {
		if (-1 == pipe(fd)) {
			goto error;
		}
	}

	/* program */
	pid = proc_rfork(0);
	switch (pid) {
	case -1:
		goto error;

	case 0:
		assert(argc >= 1);
		assert(args != NULL);

		if (type == CODE_TICK) {
			close(fd[0]);

			if (-1 == dup2(fd[1], fileno(stdout))) {
				goto error;
			}
		}

		(void) proc_exec(args[0], args);
		abort();

	default:
		break;
	}

#ifdef __GNUC__
	tick = NULL;
#endif

	if (type == CODE_TICK) {
		ssize_t r;
		char c;

		close(fd[1]);

		tick = tmpfile();
		if (tick == NULL) {
			goto error;
		}

		/* XXX: read a block at a time */
		/* XXX: centralise fcat() */
		while (r = read(fd[0], &c, 1), r == 1) {
			if (EOF == fputc(c, tick)) {
				goto error;
			}
		}

		if (r == -1) {
			goto error;
		}

		close(fd[0]);
	}

	if (-1 == proc_wait(pid)) {
/* TODO: lots of cleanup on errors all over this function. split it up */
		goto error;
	}

	/* if this is CODE_TICK, rewind() tmpfile(), fread() and push to *data stack */
	/* maybe ` can be implemented in terms of pipes in general */
	if (type == CODE_TICK) {
		char *s, *e;
		long l;
		int c;

		l = ftell(tick);
		if (l == -1) {
			goto error;
		}

		rewind(tick);

		s = malloc(l);
		if (s == NULL) {
			goto error;
		}

		for (e = s; c = fgetc(tick), c != EOF; e++) {
			*e = c;
		}

		if (ferror(tick)) {
			goto error;
		}

		fclose(tick);

		/* TODO: tokenise by $^ here, e.g. { echo `date | wc } */
		/* TODO: can tokenisation be shared with the lexer's guts exposed? */
		/* TODO: the lexer should use $^ */

		if (debug & DEBUG_VAR) {
			fprintf(stderr, "` read: %.*s\n", (int) l, s);
		}

		/* TODO: maybe have make_args output p, cut off the arg list, and data_free() it */
		for (p = *data; p->s != NULL; p = pnext) {
			assert(p != NULL);

			pnext = p->next;
			free(p);
		}

		*data = p->next;
		free(p);

		if (!data_push(data, l, s)) {
			goto error;
		}

		free(s);

		return 0;
	}

done:

	/* TODO: maybe have make_args output p, cut off the arg list, and data_free() it */
	for (p = *data; p->s != NULL; p = pnext) {
		assert(p != NULL);

		pnext = p->next;
		free(p);
	}

	*data = p->next;
	free(p);

	return 0;

error:

	perror(args[0]);

	free(args);

	return -1;
}

/* TODO: explain what happens here: status.r is the predicate, CODE_ANON is a block to call */
static int
eval_if(const struct code **next, struct rtrn **rtrn, struct data **data)
{
	const struct code *a;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	if (*next == NULL) {
		errno = 0;
		return -1;
	}

	a = *next;
	if (a->type != CODE_ANON) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * When the status condition is true, we simply leave the forthcoming node
	 * on the code stack to evaulate next. Otherwise, it is consumed and
	 * discarded here.
	 */

	if (status.r != EXIT_SUCCESS) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "discarding anon\n");
		}

		if (debug & DEBUG_STACK) {
			fprintf(stderr, "code <- %s %s\n", code_name(a->type),
				a->type == CODE_DATA ? a->u.s : "");
		}

		*next = a->next;
	}

	return 0;
}

/* TODO: explain what happens here */
static int
eval_set(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame)
{
	struct data *a;
	const struct code *b;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	if (*next == NULL || *data == NULL) {
		errno = 0;
		return -1;
	}

	a = *data;
	b = *next;

	if (b->type != CODE_ANON) {
		errno = 0;
		return -1;
	}

	if (!frame_set(frame, strlen(a->s), a->s, b->u.code)) {
		errno = EINVAL;
		return -1;
	}

	*data = a->next;
	free(a);

	*next = b->next;

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

static int
eval_binop(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame,
	int (*op)(struct data **, struct frame *, struct data *a, struct data *b))
{
	struct data *a;
	struct data *b;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(op != NULL);

	(void) next;

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

	if (-1 == op(&b->next, frame, a, b)) {
		return -1;
	}

	*data = b->next;
	free(a);
	free(b);

	return 0;
}

int
eval(const struct code *code, struct data **data)
{
	const struct code *node, *next;
	struct rtrn *rtrn;
	int r;

	assert(data != NULL);

	rtrn = NULL;

	next = code;

	while (node = next, node != NULL) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "code: "); code_dump(stderr,  node);
			fprintf(stderr, "data: "); data_dump(stderr, *data);
		}

		next = node->next;

		if (rtrn != NULL) {
			rtrn->code = next;
		}

		switch (node->type) {
		case CODE_NULL: r = eval_null(&next, &rtrn, data);                            break;
		case CODE_ANON: r = eval_anon(&next, &rtrn, data, node->u.code);              break;
		case CODE_RET:  r = eval_ret (&next, &rtrn, data);                            break;
		case CODE_DATA: r = eval_data(&next, &rtrn, data, node->u.s);                 break;
		case CODE_NOT:  r = eval_not (&next, &rtrn, data);                            break;
		case CODE_IF:   r = eval_if  (&next, &rtrn, data);                            break;
		case CODE_CALL: r = eval_call(&next, &rtrn, data, node->frame, node->u.code); break;
		case CODE_TICK: r = eval_exec(&next, &rtrn, data, node->frame, node->type);   break;
		case CODE_EXEC: r = eval_exec(&next, &rtrn, data, node->frame, node->type);   break;
		case CODE_SET:  r = eval_set (&next, &rtrn, data, node->frame);               break;

		case CODE_JOIN: r = eval_binop(&next, &rtrn, data, node->frame, op_join);     break;
		case CODE_PIPE: r = eval_binop(&next, &rtrn, data, node->frame, op_pipe);     break;

		default:
			errno = EINVAL;
			return -1;
		}

		if (r == -1) {
			return -1;
		}
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval out: ");
		data_dump(stderr, *data);
	}

	return 0;
}

