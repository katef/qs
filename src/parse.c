#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"
#include "ast.h"
#include "exec.h"
#include "parse.h"

static int
parse_block(int *t, const char **s, const char **e,
	struct ast_node **node_out, int execute);

/*
 * <list>
 *   : { <str> }
 *   ;
 */ 
static int
parse_list(int *t, const char **s, const char **e,
	struct ast_list **list_out)
{
	struct ast_list **next;
	int err;

	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);
	assert(list_out != NULL);

	next = list_out;

	while (*t == tok_str) {
		*next = ast_new_list(*e - *s, *s);
		if (*next == NULL) {
			goto error;
		}

		next = &(*next)->next;

		*t = lex_next(s, e);
		if (*t == -1) {
			goto error;
		}
	}

	if (debug & DEBUG_PARSE) {
		fprintf(stderr, "list -> { str } ;\n"); /* TODO: show actual strs */
	}

	*next = NULL;

	return 0;

error:

	err = errno;

	ast_free_list(*list_out);

	errno = err;

	return -1;
}

/*
 * <cmd>
 *   : "{" <block> "}"
 *   | <list>
 *   ;
 */
static int
parse_cmd(int *t, const char **s, const char **e,
	struct ast_node **node_out, int execute)
{
	struct ast_list *list;
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

		if (-1 == parse_block(t, s, e, &child, 0)) {
			return -1;
		}

		if (*t != tok_cbrace) {
			return -1;
		}

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "cmd -> \"{\" block \"}\" ;\n");
		}

		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		if (child == NULL) {
			return 0;
		}

		*node_out = ast_new_node_node(child);
		if (*node_out == NULL) {
			return -1;
		}

		break;

	default:
		if (-1 == parse_list(t, s, e, &list)) {
			return -1;
		}

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "cmd -> list ;\n");
		}

		if (list == NULL) {
			*node_out = NULL;

			return 0;
		}

		*node_out = ast_new_node_list(list);
		if (*node_out == NULL) {
			return -1;
		}

		break;
	}

	if (execute) {
		if (debug & DEBUG_AST) {
			ast_dump(*node_out);
		}

		exec_node(*node_out);

		ast_free_node(*node_out);

		*node_out = NULL;
	}

	return 0;
}

/*
 * <block>
 *   : <cmd> { ( ";" | "\n" ) <cmd> }
 *   ;
 */
static int
parse_block(int *t, const char **s, const char **e,
	struct ast_node **node_out, int execute)
{
	struct ast_node **out;
	int n;
	int err;

	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);

	if (-1 == parse_cmd(t, s, e, node_out, execute)) {
		return -1;
	}

	assert(*node_out == NULL || (*node_out)->next == NULL);

	out = node_out;

	for (n = 0; *t == tok_semi || *t == tok_nl; n++) {
		*t = lex_next(s, e);
		if (*t == -1) {
			goto error;
		}

		if (*out != NULL) {
			out = &(*out)->next;
		}

		if (-1 == parse_cmd(t, s, e, out, execute)) {
			goto error;
		}
	}

	if (debug & DEBUG_PARSE) {
		fprintf(stderr, "block -> cmd");

		while (n--) {
			fprintf(stderr, " \";\" cmd");
		}

		fprintf(stderr, " ;\n");
	}

	return 0;

error:

	err = errno;

	ast_free_node(*node_out);

	errno = err;

	return -1;
}

/*
 * <entry>
 *   : <block> <eof>
 *   ;
 */
static int
parse_entry(int *t, const char **s, const char **e,
	struct ast_node **node_out)
{
	assert(t != NULL && *t != -1);
	assert(s != NULL && *s != NULL);
	assert(e != NULL && *e != NULL);

	/* TODO: passing execute here would only be 1 for interactive shells */
	if (-1 == parse_block(t, s, e, node_out, 1)) {
		return -1;
	}

	if (*t == tok_eof) {
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "entry -> block eof ;\n");
		}

		return 0;
	}

	errno = 0;

	return -1;
}

int
parse(struct ast_node **node_out)
{
	const char *s, *e;
	int t;

	assert(node_out != NULL);

	t = lex_next(&s, &e);
	if (t == -1) {
		return -1;
	}

	assert(s != NULL);
	assert(e != NULL);

	if (-1 == parse_entry(&t, &s, &e, node_out)) {
		return -1;
	}

	return 0;
}

