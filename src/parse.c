#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lex.h"
#include "ast.h"
#include "parse.h"

static int
parse_list(int *t, const char **s, const char **e,
	struct ast_node **node_out);

/*
 * <exec>
 *   : <str> { <str> }
 *   ;
 */ 
static int
parse_exec(int *t, const char **s, const char **e,
	struct ast_exec **exec_out)
{
	struct ast_arg  *arg;
	struct ast_arg  **next;

	/* TODO: possibly e and s can just be const char * */

	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);
	assert(exec_out != NULL);

	if (*t != tok_str) {
		return -1;
	}

	*exec_out = ast_new_exec(*e - *s, *s);
	if (*exec_out == NULL) {
		return -1;
	}

	next = &(*exec_out)->arg;

	assert(*next == NULL);

	for (;;) {
		*t = lex_next(s, e);
		if (*t == -1) {
			/* TODO: free *exec_out here */

			return -1;
		}

		if (*t != tok_str) {
			break;
		}

		arg = ast_new_arg(*e - *s, *s);
		if (arg == NULL) {
			/* TODO: free *exec_out here */

			return -1;
		}

		*next = arg;
		next = &arg->next;
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
parse_cmd(int *t, const char **s, const char **e,
	struct ast_node **node_out)
{
	struct ast_exec *exec;
	struct ast_node *child;

	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);
	assert(node_out != NULL);

	switch (*t) {
	case tok_obrace:
		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		if (-1 == parse_list(t, s, e, &child)) {
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

		*node_out = ast_new_node_node(child);
		if (*node_out == NULL) {
			return -1;
		}

		return 0;

	default:
		if (-1 == parse_exec(t, s, e, &exec)) {
			break;
		}

		fprintf(stderr, "<cmd> : <exec> ;\n");

		*node_out = ast_new_node_exec(exec);
		if (*node_out == NULL) {
			return -1;
		}

		return 0;
	}

	if (errno == 0) {
		fprintf(stderr, "<cmd> : ;\n");

		*node_out = NULL;

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
parse_list(int *t, const char **s, const char **e,
	struct ast_node **node_out)
{
	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);

	if (-1 == parse_cmd(t, s, e, node_out)) {
		return -1;
	}

	assert(*node_out == NULL || (*node_out)->next == NULL);

	if (*t != tok_semi && *t != tok_nl) {
		fprintf(stderr, "<list> : <cmd> ;\n");

		return 0;
	}

	*t = lex_next(s, e);
	if (*t == -1) {
		/* TODO: free *node_out */

		return -1;
	}

	/* TODO: this would probably be be saner as a loop */
	if (0 == parse_list(t, s, e, *node_out == NULL ? node_out : &(*node_out)->next)) {
		fprintf(stderr, "<list> : <cmd> \"%c\" <list> ;\n", *t);

		return 0;
	}

	/* TODO: free *node_out */

	errno = 0;

	return -1;
}

/*
 * <entry>
 *   : <list> <eof>
 *   ;
 */
static int
parse_entry(int *t, const char **s, const char **e,
	struct ast_node **node_out)
{
	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);

	if (-1 == parse_list(t, s, e, node_out)) {
		return -1;
	}

	if (*t == tok_eof) {
		fprintf(stderr, "<entry> : <list> <eof> ;\n");

		return 0;
	}

	/* TODO: free child */

	errno = 0;

	return -1;
}

struct ast_node *
parse(void)
{
	struct ast_node *node;
	const char *s, *e;
	int t;

	t = lex_next(&s, &e);
	if (t == -1) {
		return NULL;
	}

	assert(s != NULL);
	assert(e != NULL);

	if (-1 == parse_entry(&t, &s, &e, &node)) {
		return NULL;
	}

	return node;
}

