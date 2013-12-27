#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lex.h"

static int
parse_list(int *t, const char **s, const char **e);

/*
 * <exec>
 *   : <str> { <str> }
 *   ;
 */ 
static int
parse_exec(int *t, const char **s, const char **e)
{
	if (*t != tok_str) {
		return -1;
	}

	for (;;) {
		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		if (*t != tok_str) {
			break;
		}

		/* TODO: ast_new_cmd() should emit &cmd->next for the arg list */
		/* TODO: write arg to *next here, at end of list */
	}

	fprintf(stderr, "<exec> : <str> { <str> } ;\n"); /* TODO: show actual strs */

	return 0;
}

/*
 * <cmd>
 *   : "{" <block> "}"
 *   | <exec>
 *   |
 *   ;
 */
static int
parse_cmd(int *t, const char **s, const char **e)
{
	switch (*t) {
	case tok_obrace:
		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		if (-1 == parse_list(t, s, e)) {
			return -1;
		}

		if (*t != tok_cbrace) {
			return -1;
		}

		fprintf(stderr, "<cmd> : \"{\" <block> \"}\" ;\n");

		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		return 0;

	default:
		if (-1 == parse_exec(t, s, e)) {
			break;
		}

		fprintf(stderr, "<cmd> : <exec> ;\n");

		return 0;
	}

	if (errno == 0) {
		fprintf(stderr, "<cmd> : ;\n");

		return 0;
	}

	return -1;
}

/*
 * <list>
 *   : <cmd> [ ( ";" | "\n" ) <list> ]
 *   ;
 */
static int
parse_list(int *t, const char **s, const char **e)
{
	if (-1 == parse_cmd(t, s, e)) {
		return -1;
	}

	if (*t != tok_semi && *t != tok_nl) {
		fprintf(stderr, "<list> : <cmd> ;\n");

		return 0;
	}

	*t = lex_next(s, e);
	if (*t == -1) {
		return -1;
	}

	if (0 == parse_list(t, s, e)) {
		fprintf(stderr, "<list> : <cmd> \"%c\" <list> ;\n", *t);

		return 0;
	}

	errno = 0;

	return -1;
}

/*
 * <entry>
 *   : <list> <eof>
 *   ;
 */
static int
parse_entry(int *t, const char **s, const char **e)
{
	if (-1 == parse_list(t, s, e)) {
		return -1;
	}

	if (*t == tok_eof) {
		fprintf(stderr, "<entry> : <list> <eof> ;\n");

		return 0;
	}

	errno = 0;

	return -1;
}

static int
parse(void)
{
	const char *s, *e;
	int t;

	t = lex_next(&s, &e);
	if (t == -1) {
		return -1;
	}

	/* TODO: pass &ast as argument to populate */
	return parse_entry(&t, &s, &e);
}

int
main(void)
{
	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	if (-1 == parse()) {
		perror("parse");
		/* TODO: free ast */
		return -1;
	}

	/* TODO: do something with ast */

	/* TODO: free ast */

	return 0;
}

