/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"
#include "proc.h"
#include "code.h"
#include "data.h"
#include "frame.h"
#include "parser.h"
#include "status.h"
#include "signal.h"
#include "args.h"
#include "eval.h"
#include "hook.h"
#include "pair.h"
#include "task.h"
#include "var.h"

unsigned debug;

static struct task *tasks;
static struct frame *top;

static int
debug_flags(const char *s)
{
	int v;

	assert(s != NULL);

	v = 1;

	for ( ; *s != '\0'; s++) {
		int e;

		switch (*s) {
		case '+': v = 1; continue;
		case '-': v = 0; continue;

		case 'a': e = ~0U;         break;
		case 'b': e = DEBUG_BUF;   break;
		case 'l': e = DEBUG_LEX;   break;
		case 'p': e = DEBUG_PARSE; break;
		case 'c': e = DEBUG_ACT;   break;
		case 'f': e = DEBUG_FRAME; break;
		case 's': e = DEBUG_STACK; break;
		case 'v': e = DEBUG_VAR;   break;
		case 'e': e = DEBUG_EVAL;  break;
		case 'x': e = DEBUG_EXEC;  break;
		case 'r': e = DEBUG_PROC;  break;
		case 'g': e = DEBUG_SIG;   break;
		case 'd': e = DEBUG_FD;    break;

		default:
			fprintf(stderr, "-d: unrecognised flag '%c'\n", *s);
			return -1;
		}

		if (v) {
			debug |=  e;
		} else {
			debug &= ~e;
		}
	}

	return 0;
}

static int
dispatch(struct code *code)
{
	int r;
	int e;

	if (code == NULL) {
		return 0;
	}

	if (!task_add(&tasks, top, code)) {
		return -1;
	}

	/*
	 * The top frame is kept around even when the last task exits,
	 * so that it can persist across multiple dispatches.
	 */
	(void) frame_refcount(top, +1);

	r = eval(&tasks);
	if (r == -1) {
		e = errno;
	}

	(void) frame_refcount(top, -1);

	if (r == -1) {
		errno = e;
	}

	return r;
}

int
hook(const char *s)
{
	struct code *code;
	struct task *new;
	struct var *v;

	assert(0 == (strncmp)(s, "on", 2) || 0 == (strncmp)(s, "sig", 3));

	v = frame_get(top, strlen(s), s);
	if (v == NULL) {
		return 0;
	}

	code = NULL;

	/* XXX: share guts with eval_anon */
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "wind code from hook $%s: ", s);
		code_dump(stderr, v->code);
	}

	if (-1 == code_wind(&code, v->code)) {
		return -1;
	}

	new = task_add(&tasks, top, code);
	if (new == NULL) {
		/* TODO: free code */
		return -1;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct lex_state l;

	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	l.f = stdin;
	l.p = NULL;
	l.pos.line = 0;
	l.pos.col  = 0;

	{
		int c;

		while (c = getopt(argc, argv, "d:"), c != -1) {
			switch (c) {
			case 'd':
				if (-1 == debug_flags(optarg)) {
					goto usage;
				}
				break;

			default:
				goto usage;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (-1 == sig_init()) {
		return 1;
	}

	{
		struct frame *q;
		struct lex_mark m;

		/* TODO: in set_args, construct string from argv[] and .col per arg */
		m.buf = "";
		m.pos.line = 0;
		m.pos.col  = 0;

		if (!frame_push(&top)) {
			return 1;
		}

		/* TODO: populate $^ here */

		if (!pair_push(&top->asc, STDOUT_FILENO, STDIN_FILENO)) {
			/* TODO: free frame */
			goto error;
		}

		if (-1 == set_args(&m, top, argv)) {
			/* TODO: free frame */
			goto error;
		}

		if (-1 == parse(&l, dispatch)) {
			goto error;
		}

		q = frame_pop(&top);
		frame_free(q);
		assert(top == NULL);
	}

	if (-1 == sig_fini()) {
		return 1;
	}

	proc_exit(status.r);

error:

	/* TODO: cleanup. maybe atexit */

	proc_exit(1);

usage:

	fprintf(stderr, "usage: qs [-d +-ablpcfsvexrd]\n");

	return 1;
}

