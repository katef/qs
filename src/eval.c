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

struct pipe_state {
	int fd[2];
	int usein, useout;
	int in, out;
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

/* push current code pointer to rtrn stack; set current code pointer to anon block */
static int
eval_anon(const struct code **next, struct rtrn **rtrn, const struct code *ci)
{
	assert(next != NULL);
	assert(rtrn != NULL);

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s\n", code_name(CODE_DATA));
	}

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "push return from: ");
		code_dump(stderr, *next);
	}

	if (!rtrn_push(rtrn, *next)) {
		return -1;
	}

	*next = ci;

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

/* push .s=NULL to data */
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

static int
eval_end(struct pipe_state *ps)
{
	ps->usein  = ps->in;
	ps->in     = -1;

	/* XXX: close() previous pipe here (this triggers waitpid) */

	/* XXX: i don't like hardcoded knowledge of STDIN_FILENO and STDOUT_FILENO here */
	ps->useout = STDOUT_FILENO;

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

	*data = a->next;
	free(a);

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
eval_exec(const struct code **next, struct rtrn **rtrn, struct data **data,
	struct frame *frame, enum code_type type, struct pipe_state *ps)
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

		/* XXX: i don't like hardcoded knowledge of STDIN_FILENO and STDOUT_FILENO here */

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

	/* XXX: but i don't want to waitpid for a multiple pipeline; wait for them all at the end.
	 * maybe make a #wait perhaps? */
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

	a = *data;

	if (!frame_set(frame, strlen(a->s), a->s, code)) {
		errno = EINVAL;
		return -1;
	}

	*data = a->next;
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
	struct pipe_state ps;
	struct rtrn *rtrn;
	int r;

	assert(data != NULL);

	rtrn = NULL;

	next = code;

	/* XXX: this should be the only place where STDIN_FILENO and STDOUT_FILENO are set */
	/* XXX: setting all of these is not neccessary. some should be -1 */
	ps.in  = ps.usein  = STDIN_FILENO;
	ps.out = ps.useout = STDOUT_FILENO;

	while (node = next, node != NULL) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "code: "); code_dump(stderr,  node);
			fprintf(stderr, "data: "); data_dump(stderr, *data);
		}

		next = node->next;

		switch (node->type) {
		case CODE_NULL: r = eval_null(data);                                             break;
		case CODE_ANON: r = eval_anon(&next, &rtrn, node->u.code);                       break;
		case CODE_RET:  r = eval_ret (&next, &rtrn);                                     break;
		case CODE_DATA: r = eval_data(data, node->u.s);                                  break;
		case CODE_PIPE: r = eval_pipe(&ps);                                              break;
		case CODE_END:  r = eval_end(&ps);                                               break;
		case CODE_NOT:  r = eval_not ();                                                 break;
		case CODE_IF:   r = eval_if  (&next, &rtrn, data, node->u.code);                 break;
		case CODE_CALL: r = eval_call(&next, &rtrn, data, node->frame);                  break;
		case CODE_TICK: r = eval_exec(&next, &rtrn, data, node->frame, node->type, &ps); break;
		case CODE_EXEC: r = eval_exec(&next, &rtrn, data, node->frame, node->type, &ps); break;
		case CODE_SET:  r = eval_set (&rtrn, data, node->frame, node->u.code);           break;

		case CODE_JOIN: r = eval_binop(&rtrn, data, node->frame, op_join);               break;

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

