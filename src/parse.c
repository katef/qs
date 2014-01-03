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
parse_block(struct lex_state *l, struct lex_tok *t,
	struct ast_node **node_out, int execute);

/*
 * <list>
 *   : { <str> }
 *   ;
 */ 
static int
parse_list(struct lex_state *l, struct lex_tok *t,
	struct ast_list **list_out)
{
	struct ast_list **next;
	int e;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_panic);
	assert(list_out != NULL);

	next = list_out;

	while (t->type == tok_str) {
		*next = ast_new_list(t->e - t->s, t->s);
		if (*next == NULL) {
			goto error;
		}

		next = &(*next)->next;

		lex_next(l, t);

		if (t->type == tok_error || t->type == tok_panic) {
			goto error;
		}
	}

	if (debug & DEBUG_PARSE) {
		fprintf(stderr, "list -> { str } ;\n"); /* TODO: show actual strs */
	}

	*next = NULL;

	return 0;

error:

	e = errno;

	ast_free_list(*list_out);

	errno = e;

	return -1;
}

/*
 * <cmd>
 *   : "{" <block> "}"
 *   | <list>
 *   ;
 */
static int
parse_cmd(struct lex_state *l, struct lex_tok *t,
	struct ast_node **node_out, int execute)
{
	struct ast_list *list;
	struct ast_node *child;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_panic);
	assert(node_out != NULL);

	switch (t->type) {
	case tok_obrace:
		lex_next(l, t);

		if (t->type == tok_error || t->type == tok_panic) {
			return -1;
		}

		if (-1 == parse_block(l, t, &child, 0)) {
			return -1;
		}

		if (t->type != tok_cbrace) {
			return -1;
		}

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "cmd -> \"{\" block \"}\" ;\n");
		}

		lex_next(l, t);

		if (t->type == tok_error || t->type == tok_panic) {
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
		if (-1 == parse_list(l, t, &list)) {
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
parse_block(struct lex_state *l, struct lex_tok *t,
	struct ast_node **node_out, int execute)
{
	struct ast_node **out;
	int n;
	int e;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_panic);

	if (-1 == parse_cmd(l, t, node_out, execute)) {
		return -1;
	}

	assert(*node_out == NULL || (*node_out)->next == NULL);

	out = node_out;

	for (n = 0; t->type == tok_semi || t->type == tok_nl; n++) {
		lex_next(l, t);

		if (t->type == tok_error || t->type == tok_panic) {
			return -1;
		}

		if (*out != NULL) {
			out = &(*out)->next;
		}

		if (-1 == parse_cmd(l, t, out, execute)) {
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

	e = errno;

	ast_free_node(*node_out);

	errno = e;

	return -1;
}

/*
 * <entry>
 *   : <block> <eof>
 *   ;
 */
static int
parse_entry(struct lex_state *l, struct lex_tok *t,
	struct ast_node **node_out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_panic);

	/* TODO: passing execute here would only be 1 for interactive shells */
	if (-1 == parse_block(l, t, node_out, 1)) {
		return -1;
	}

	if (t->type == tok_eof) {
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "entry -> block eof ;\n");
		}

		return 0;
	}

	errno = 0;

	return -1;
}

int
parse(struct lex_state *l, struct ast_node **node_out)
{
	struct lex_tok t;

	assert(l != NULL);
	assert(l->f != NULL);
	assert(node_out != NULL);

	l->p  = l->buf;

	do {
		lex_next(l, &t);

		if (-1 == parse_entry(l, &t, node_out)) {
			return -1;
		}
	} while (t.type == tok_panic);

	if (t.type == tok_error) {
		return -1;
	}

	return 0;
}

