#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lex.h"
#include "ast.h"
#include "parse.h"

static int
parse_list(int *t, const char **s, const char **e,
	struct ast_node **node_out, int execute);

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
	int err;

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
			goto error;
		}

		if (*t != tok_str) {
			break;
		}

		arg = ast_new_arg(*e - *s, *s);
		if (arg == NULL) {
			goto error;
		}

		*next = arg;
		next = &arg->next;
	}

	fprintf(stderr, "exec -> str { str } ;\n"); /* TODO: show actual strs */

	return 0;

error:

	err = errno;

	ast_free_exec(*exec_out);

	errno = err;

	return -1;
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
	struct ast_node **node_out, int execute)
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

		if (-1 == parse_list(t, s, e, &child, 0)) {
			return -1;
		}

		if (*t != tok_cbrace) {
			return -1;
		}

		fprintf(stderr, "cmd -> \"{\" block \"}\" ;\n");

		*t = lex_next(s, e);
		if (*t == -1) {
			return -1;
		}

		*node_out = ast_new_node_node(child);
		if (*node_out == NULL) {
			return -1;
		}

		goto accept;

	default:
		if (-1 == parse_exec(t, s, e, &exec)) {
			break;
		}

		fprintf(stderr, "cmd -> exec ;\n");

		*node_out = ast_new_node_exec(exec);
		if (*node_out == NULL) {
			return -1;
		}

		goto accept;
	}

	if (errno == 0) {
		fprintf(stderr, "cmd -> ;\n");

		*node_out = NULL;

		return 0;
	}

	return -1;

accept:

	if (execute) {
		ast_dump(*node_out);

		ast_free_node(*node_out);

		*node_out = NULL;
	}

	return 0;
}

/*
 * <list>
 *   : <cmd> { ( ";" | "\n" ) <cmd> }
 *   ;
 */
static int
parse_list(int *t, const char **s, const char **e,
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

	fprintf(stderr, "list -> cmd");

	while (n--) {
		fprintf(stderr, " \";\" cmd");
	}

	fprintf(stderr, " ;\n");

	return 0;

error:

	err = errno;

	ast_free_node(*node_out);

	errno = err;

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

	/* TODO: passing execute here would only be 1 for interactive shells */
	if (-1 == parse_list(t, s, e, node_out, 1)) {
		return -1;
	}

	if (*t == tok_eof) {
		fprintf(stderr, "entry -> list eof ;\n");

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

