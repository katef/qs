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
#include "readfd.h"
#include "status.h"
#include "builtin.h"

struct pipe_state {
	int fd[2];
	int usein, useout;
	int in, out;
};

struct tick_state {
	/* TODO: decide where to store the growing block. need to keep state for re-entry */
	/* XXX: s needs to be kept statefully elsewhere. need state for re-entry */
	char *s;
	size_t l;
};

/* push .s=NULL to data */
static int
eval_null(struct data **data)
{
	assert(data != NULL);

	if (!data_push(data, 0, NULL)) {
		return -1;
	}

	return 0;
}

/* pop from return stack; set current code pointer to that */
static int
eval_ret(const struct code **next, struct rtrn **rtrn)
{
	struct rtrn *a;

	assert(next != NULL);
	assert(rtrn != NULL);

	a = rtrn_pop(rtrn);
	if (a == NULL) {
		errno = 0;
		return -1;
	}

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "return to: ");
		code_dump(stderr, a->code);
	}

	*next = a->code;

	free(a);

	return 0;
}

/* push .s="xyz" to data */
static int
eval_data(struct data **data, const char *s)
{
	assert(data != NULL);
	assert(s != NULL);

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s \"%s\"\n", code_name(CODE_DATA), s);
	}

	if (!data_push(data, strlen(s), s)) {
		return -1;
	}

	return 0;
}

static int
eval_pipe(struct pipe_state *ps)
{
	assert(ps != NULL);
	assert(ps->in != -1);

	if (-1 == pipe(ps->fd)) {
		return -1;
	}

	if (debug & DEBUG_FD) {
		fprintf(stderr, "pipe [%d,%d]\n", ps->fd[0], ps->fd[1]);
	}

	/* XXX: close() previous pipe here (this triggers waitpid) */

	ps->usein  = ps->in;
	ps->in     = ps->fd[0];

	ps->useout = ps->fd[1];

	return 0;
}

/* pop $?, push !$? to *data */
static int
eval_not(void)
{
	status_exit(!status.r);

	return 0;
}

/* pop variable name; wind from variable to *data and next */
static int
eval_call(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame)
{
	const struct var *q;
	struct data *a;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	if (*data == NULL) {
		errno = 0;
		return -1;
	}

	a = data_pop(data);
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
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "push return from: ");
		code_dump(stderr, *next);
	}

	if (!rtrn_push(rtrn, *next)) {
		return -1;
	}

	*next = q->code;

	return 0;
}

/* eat whole data stack upto .s=NULL, make argv and execute */
static int
eval_run(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame, struct pipe_state *ps)
{
	struct data *p, *pnext;
	struct var *v;
	pid_t pid;
	int argc;
	char **args;

	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(ps != NULL);

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
		if (debug & DEBUG_STACK) {
			fprintf(stderr, "push return from: ");
			code_dump(stderr, *next);
		}

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

	/* program */
	pid = proc_rfork(0);
	switch (pid) {
	case -1:
		goto error;

	case 0:
		assert(argc >= 1);
		assert(args != NULL);

		/* XXX: i don't like hardcoded knowledge of STDIN_FILENO and STDOUT_FILENO here */
		/* TODO: error handling for dup2, here and elsewhere */

		if (ps->usein != STDIN_FILENO) {
			dup2(ps->usein, STDIN_FILENO);

			if (debug & DEBUG_FD) {
				fprintf(stderr, "child: dup2(%d, %d)\n", ps->usein, STDIN_FILENO);
			}

			close(ps->usein);
		}

		if (ps->useout != STDOUT_FILENO) {
			dup2(ps->useout, STDOUT_FILENO);

			if (debug & DEBUG_FD) {
				fprintf(stderr, "child: dup2(%d, %d)\n", ps->useout, STDOUT_FILENO);
			}

			close(ps->useout);
		}

		(void) proc_exec(args[0], args);
		abort();

	default:
		break;
	}

	/* TODO: associate waiting PID with code block for cooperative re-entry */

	return 0;

done:

	/* XXX: why not data_free here? */

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

/* read from the pipe and push to *data */
/* note will need to be able to re-enter here after EINTR */
static int
eval_tick(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame, struct pipe_state *ps, struct tick_state *ts)
{
	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(ps != NULL);
	assert(ts != NULL);

	{
		int r;

		r = readfd(ps->in, &ts->s, &ts->l);

		if (r == -1 && errno == EINTR) {
			return 0;
		}

		if (r == -1) {
			return -1;
		}

		if (debug & DEBUG_VAR) {
			fprintf(stderr, "` read: %.*s\n", (int) ts->l, ts->s);
		}
	}

	/* TODO: tokenise by $^ here, e.g. { echo `date | wc } */
	/* TODO: can tokenisation be shared with the lexer's guts exposed? */
	/* TODO: the lexer should use $^ */
	/* TODO: \0 should implicitly always be in the $^ set */
	/* TODO: after reading, always split by '\0'. dealing with $^ is just s//ing more '\0' in situ */
	/* TODO: push each token as a separate item to the data stack */

	if (!data_push(data, ts->l, ts->s)) {
		goto error;
	}

	free(ts->s);

ts->s = NULL; /* XXX: doesn't belong here */

	return 0;

error:

	free(ts->s);

ts->s = NULL; /* XXX: doesn't belong here */

	return -1;
}

/* TODO: explain what happens here: status.r is the predicate, u.code is a block to call */
static int
eval_if(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct code *code)
{
	assert(next != NULL);
	assert(rtrn != NULL);
	assert(data != NULL);

	/*
	 * When the status condition is true, we simply ignore the u.code pointer.
	 * Otherwise, the current code is pushed to the return stack, and control
	 * transfered to the code passed.
	 */

	if (status.r == EXIT_SUCCESS) {
		/* XXX: share guts with eval_anon */
		if (debug & DEBUG_STACK) {
			fprintf(stderr, "push return from: ");
			code_dump(stderr, *next);
		}

		if (!rtrn_push(rtrn, *next)) {
			return -1;
		}

		*next = code;
	}

	return 0;
}

/* TODO: explain what happens here */
static int
eval_set(struct rtrn **rtrn, struct data **data,
	struct frame *frame, struct code *code)
{
	struct data *a;

	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	if (*data == NULL) {
		errno = 0;
		return -1;
	}

	a = data_pop(data);

	if (!frame_set(frame, strlen(a->s), a->s, code)) {
		errno = EINVAL;
		return -1;
	}

	free(a);

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

static int
eval_binop(struct rtrn **rtrn, struct data **data,
	struct frame *frame,
	int (*op)(struct data **, struct frame *, struct data *a, struct data *b))
{
	struct data *a;
	struct data *b;

	assert(rtrn != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(op != NULL);

	if (*data == NULL || (*data)->next == NULL) {
		errno = 0;
		return -1;
	}

	a = data_pop(data);
	b = data_pop(data);
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data <- %s\n", a->s);
		fprintf(stderr, "data <- %s\n", b->s);
	}

	if (-1 == op(data, frame, a, b)) {
		return -1;
	}

	free(a);
	free(b);

	return 0;
}

int
eval(const struct code *code, struct data **data)
{
	const struct code *node, *next;
	struct pipe_state ps;
	struct tick_state ts;
	struct rtrn *rtrn;
	int r;

	assert(data != NULL);

	rtrn = NULL;

	next = code;

	/* XXX: this should be the only place where STDIN_FILENO and STDOUT_FILENO are set */
	/* XXX: setting all of these is not neccessary. some should be -1 */
	ps.in  = ps.usein  = STDIN_FILENO;
	ps.out = ps.useout = STDOUT_FILENO;

	/* XXX: will need an independent ts per context. ditto for pipe state */
	ts.s = NULL;

	while (node = next, node != NULL) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "code: "); code_dump(stderr,  node);
			fprintf(stderr, "data: "); data_dump(stderr, *data);
		}

		next = node->next;

		switch (node->type) {
		case CODE_NULL: r = eval_null(data);                                      break;
		case CODE_RET:  r = eval_ret (&next, &rtrn);                              break;
		case CODE_DATA: r = eval_data(data, node->u.s);                           break;
		case CODE_PIPE: r = eval_pipe(&ps);                                       break;
		case CODE_NOT:  r = eval_not ();                                          break;
		case CODE_IF:   r = eval_if  (&next, &rtrn, data, node->u.code);          break;
		case CODE_CALL: r = eval_call(&next, &rtrn, data, node->frame);           break;
		case CODE_TICK: r = eval_tick(&next, &rtrn, data, node->frame, &ps, &ts); break;
		case CODE_RUN:  r = eval_run (&next, &rtrn, data, node->frame, &ps);      break;
		case CODE_SET:  r = eval_set (&rtrn, data, node->frame, node->u.code);    break;
		case CODE_JOIN: r = eval_binop(&rtrn, data, node->frame, op_join);        break;

		default:
			errno = EINVAL;
			return -1;
		}

		if (r == -1) {
			return -1;
		}

		if (-1 == proc_wait(-1)) {
			perror("wait");
			continue;
		}

/* TODO: cooperative re-entry to the appropriate code stack. need PID->code lookup */
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval out: ");
		data_dump(stderr, *data);
	}

	return 0;
}

