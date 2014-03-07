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
		case 'f': debug |= DEBUG_FRAME; break;
		case 's': debug |= DEBUG_STACK; break;
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
		struct data *data;

		data = NULL;

		if (a[i].s != NULL) {
			if (!data_push(&data, 1, "_")) {
				return -1;
			}
		}

		if (!frame_set(frame, a[i].name, NULL, data)) {
			return -1;
		}
	}

	return 0;
}

static int
dispatch(FILE *f, struct frame *frame, struct code **code, struct data **data)
{
	struct data *out;

	assert(f != NULL);
	assert(code != NULL);
	assert(data != NULL);

	/* TODO: okay to have eval() modify *code, *data */
	if (-1 == eval_clone(*code, *data, &out)) {
		return -1;
	}

	if (!frame_set(frame, "_", NULL, out)) {
		return -1;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct data *args;
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

	{
		int i;

		args = NULL;

		for (i = 0; i < argc; i++) {
			if (!data_push(&args, strlen(argv[i]), argv[i])) {
				perror("data_push");
				goto error;
			}
		}
	}

	if (-1 == parse(&l, populate, dispatch, args)) {
		perror("parse");
		goto error;
	}

	/* TODO: free args */

	/* TODO: retrieve $? */

	return 0;

error:

	return 1;

usage:

	fprintf(stderr, "usage: kcsh [-d ablpfsx]\n");

	return 1;
}

