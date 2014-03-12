#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "lex.h"
#include "code.h"
#include "data.h"
#include "frame.h"
#include "parser.h"
#include "eval.h"
#include "args.h"

unsigned debug;

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
		case 'e': debug |= DEBUG_EVAL;  break;
		case 'x': debug |= DEBUG_EXEC;  break;

		default:
			fprintf(stderr, "-d: unrecognised flag '%c'\n", *s);
			return -1;
		}
	}

	return 0;
}

static int
populate(struct frame *frame)
{
	size_t i;
	static char pid[32];

	struct {
		const char *name;
		const char *s;
	} a[] = {
		{ "?", "0"  },
		{ "_", NULL },
		{ "$", pid  }, /* TODO: getpid()  */
		{ "^", "\t" }
	};

	assert(frame != NULL);

	sprintf(pid,  "%ld", (long) getpid()); /* XXX */

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		struct code *code;

		code = NULL;

		if (a[i].s != NULL) {
			if (!code_data(&code, frame, strlen(a[i].s), a[i].s)) {
				return -1;
			}
		}

		if (!frame_set(frame, strlen(a[i].name), a[i].name, code)) {
			return -1;
		}
	}

	return 0;
}

static int
dispatch(FILE *f, struct frame *frame, char *args[], struct code **code)
{
	const struct data *p;
	struct data *out;
	struct code *ci;

	assert(f != NULL);
	assert(args != NULL);
	assert(code != NULL);

	ci = NULL;

	if (-1 == set_args(frame, args)) {
		return -1;
	}

	out = NULL;

	if (-1 == eval(code, &out)) {
		return -1;
	}

	assert(*code == NULL);

	ci = NULL;

	for (p = out; p != NULL; p = p->next) {
		assert(p->s != NULL);

		if (!code_data(&ci, frame, strlen(p->s), p->s)) {
			goto error;
		}
	}

	if (!frame_set(frame, 1, "_", ci)) {
		return -1;
	}

	data_free(out);

	return 0;

error:

	code_free(ci);
	data_free(out);

	return -1;
}

int
main(int argc, char *argv[])
{
	struct lex_state l;

	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	l.f = stdin;

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

	if (-1 == parse(&l, populate, dispatch, argv)) {
		perror("parse");
		goto error;
	}

	return status;

error:

	return 1;

usage:

	fprintf(stderr, "usage: kcsh [-d ablpcfsex]\n");

	return 1;
}

