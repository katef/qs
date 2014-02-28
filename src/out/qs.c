#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"

#include "../out.h"

static int
dump_node(const struct ast *a);

static int
dump_list(const char *s, const char *e, const struct ast_list *l)
{
	const struct ast_list *p;

	assert(s != NULL);
	assert(e != NULL);

	fprintf(stderr, "%s", s);

	for (p = l; p != NULL; p = p->next) {
		switch (p->a->type) {
		case AST_STR:
			fprintf(stderr, "%s", p->a->u.s);
			break;

		default:
			fprintf(stderr, "?");
			break;
		}

		if (p->next) {
			fprintf(stderr, " ");
		}
	}

	fprintf(stderr, "%s", e);

	return 0;
}

static int
dump_block(const char *s, const char *e, const struct ast *a)
{
	assert(s != NULL);
	assert(e != NULL);
	assert(a != NULL);

	fprintf(stderr, "%s", s);
	dump_node(a->u.a);
	fprintf(stderr, "%s", e);

	return 0;
}

static int
dump_op(const char *op, const struct ast *a)
{
	assert(op != NULL);
	assert(a != NULL);

	dump_node(a->u.op.a);
	fprintf(stderr, "%s", op);
	dump_node(a->u.op.b);

	return 0;
}

static int
dump_node(const struct ast *a)
{
	if (a == NULL) {
		return 0;
	}

	switch (a->type) {
	case AST_STR:   return !fprintf(stderr, "%s", a->u.s);
	case AST_EXEC:  return dump_list("",  "",  a->u.l);
	case AST_LIST:  return dump_list("(", ")", a->u.l);

	case AST_DEREF: return dump_block("$",  "",   a);
	case AST_BLOCK: return dump_block("{ ", " }", a);
	case AST_CALL:  return dump_block("(",  ")",  a);
	case AST_SETBG: return dump_block("",   "&",  a);

	case AST_AND:    return dump_op(" && ", a);
	case AST_OR:     return dump_op(" || ", a);
	case AST_JOIN:   return dump_op("^",    a);
	case AST_PIPE:   return dump_op(" | ",  a);
	case AST_ASSIGN: return dump_op("=",    a);
	case AST_SEP:    return dump_op("; ",   a);

	default:
		fprintf(stderr, "? %d ", a->type);
		return 0;
	}
}

int
out_qs(const struct ast *a)
{
	dump_node(a);

	return 0;
}

