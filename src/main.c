#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "lex.h"
#include "ast.h"
#include "frame.h"
#include "parser.h"
#include "list.h"
#include "out.h"

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
		case 'x': debug |= DEBUG_EXEC;  break;

		default:
			fprintf(stderr, "-d: unrecognised flag '%c'\n", *s);
			return -1;
		}
	}

	return 0;
}

static int
(*out_format(const char *s))
	(FILE *, struct ast *)
{
	size_t i;

	struct {
		const char *name;
		int (*f)(FILE *, struct ast *);
	} a[] = {
		{ "eval",  out_eval  },
		{ "qs",    out_qs    },
		{ "ast",   out_ast   },
		{ "frame", out_frame }
	};

	assert(s != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(s, a[i].name)) {
			return a[i].f;
		}
	}

	return NULL;
}

static int
populate(struct frame *f)
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

	assert(f != NULL);

	sprintf(pid,  "%ld", (long) getpid()); /* XXX */

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		struct ast *q;

		if (a[i].s != NULL) {
			q = ast_new_leaf(AST_STR, strlen(a[i].s), a[i].s);
			if (q == NULL) {
				return -1;
			}
		} else {
			q = NULL;
		}

		if (!frame_set(f, a[i].name, q)) {
			return -1;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	int (*dispatch)(FILE *f, struct ast *a);
	struct ast_list *args;
	struct lex_state l;

	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	dispatch = out_eval;

	l.f = stdin;

	{
		int c;

		while (c = getopt(argc, argv, "d:e:"), c != -1) {
			switch (c) {
			case 'd':
				if (-1 == debug_flags(optarg)) {
					goto usage;
				}
				break;

			case 'e':
				dispatch = out_format(optarg);
				if (dispatch == NULL) {
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

	args = list_args(argc, argv);
	if (argc > 0 && args == NULL) {
		perror("list_args");
		goto error;
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

	fprintf(stderr, "usage: kcsh [-d ablpfx] [-e eval|qs|ast|frame]\n");

	return 1;
}

