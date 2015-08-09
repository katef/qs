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
	assert(s != NULL);

	for ( ; *s != '\0'; s++) {
		switch (*s) {
		case 'a': debug = ~0U;          break;
		case 'b': debug |= DEBUG_BUF;   break;
		case 'l': debug |= DEBUG_LEX;   break;
		case 'p': debug |= DEBUG_PARSE; break;
		case 'c': debug |= DEBUG_ACT;   break;
		case 'f': debug |= DEBUG_FRAME; break;
		case 's': debug |= DEBUG_STACK; break;
		case 'v': debug |= DEBUG_VAR;   break;
		case 'e': debug |= DEBUG_EVAL;  break;
		case 'x': debug |= DEBUG_EXEC;  break;
		case 'r': debug |= DEBUG_PROC;  break;
		case 'g': debug |= DEBUG_SIG;   break;
		case 'd': debug |= DEBUG_FD;    break;

		default:
			fprintf(stderr, "-d: unrecognised flag '%c'\n", *s);
			return -1;
		}
	}

	return 0;
}

static int
dispatch(struct code *code)
{
	int r;
	int e;

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

	{
		struct frame *q;

		if (!frame_push(&top)) {
			return 1;
		}

		/* TODO: populate $^ here */

		if (!pair_push(&top->asc, STDOUT_FILENO, STDIN_FILENO)) {
			/* TODO: free frame */
			goto error;
		}

		if (-1 == set_args(&l.pos, top, argv)) {
			/* TODO: free frame */
			goto error;
		}

		do {
			struct code *code;

			if (-1 == parse(&l, &code)) {
				perror("parse");
				goto error;
			}

			if (code == NULL) {
				continue;
			}

			if (-1 == dispatch(code)) {
				perror("dispatch");
				goto error;
			}

			assert(code == NULL);
		} while (!feof(l.f));

		q = frame_pop(&top);
		frame_free(q);
		assert(top == NULL);
	}

	proc_exit(status.r);

error:

	/* TODO: cleanup. maybe atexit */

	proc_exit(1);

usage:

	fprintf(stderr, "usage: qs [-d ablpcfsvexrd]\n");

	return 1;
}

