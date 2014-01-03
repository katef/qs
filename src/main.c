#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "ast.h"
#include "lex.h"
#include "parse.h"

unsigned debug;

static int
debug_flags(const char *s)
{
	for ( ; *s != '\0'; s++) {
		switch (*s) {
		case 'a': debug = ~0U;          break;
		case 'b': debug |= DEBUG_BUF;   break;
		case 'l': debug |= DEBUG_LEX;   break;
		case 'p': debug |= DEBUG_PARSE; break;
		case 't': debug |= DEBUG_AST;   break;
		case 'x': debug |= DEBUG_EXEC;  break;

		default:
			fprintf(stderr, "-d: unrecognised flag '%c'\n", *s);
			return -1;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct ast_node *node;
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

	if (argc != 0) {
		goto usage;
	}

	if (-1 == parse(&l, &node)) {
		perror("parse");
		goto error;
	}

	if (debug & DEBUG_AST) {
		if (-1 == ast_dump(node)) {
			perror("ast_dump");
			goto error;
		}
	}

	ast_free_node(node);

	return 0;

error:

	ast_free_node(node);

	return 1;

usage:

	fprintf(stderr, "usage: kcsh [-d ablptx]\n");

	return 1;
}

