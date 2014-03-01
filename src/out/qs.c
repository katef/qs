#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"

#include "../out.h"

static int
dump_node(FILE *f, const struct ast *a);

static int
dump_list(FILE *f, const char *s, const char *e, const struct ast_list *l)
{
	const struct ast_list *p;

	assert(f != NULL);
	assert(s != NULL);
	assert(e != NULL);

	fprintf(f, "%s", s);

	for (p = l; p != NULL; p = p->next) {
		switch (p->a->type) {
		case AST_STR:
			fprintf(f, "%s", p->a->u.s);
			break;

		default:
			fprintf(f, "?");
			break;
		}

		if (p->next) {
			fprintf(f, " ");
		}
	}

	fprintf(f, "%s", e);

	return 0;
}

static int
dump_block(FILE *f, const char *s, const char *e, const struct ast *a)
{
	assert(f != NULL);
	assert(s != NULL);
	assert(e != NULL);
	assert(a != NULL);

	fprintf(f, "%s", s);
	dump_node(f, a->u.a);
	fprintf(f, "%s", e);

	return 0;
}

static int
dump_op(FILE *f, const char *op, const struct ast *a)
{
	assert(f != NULL);
	assert(op != NULL);
	assert(a != NULL);

	dump_node(f, a->u.op.a);
	fprintf(f, "%s", op);
	dump_node(f, a->u.op.b);

	return 0;
}

static int
dump_node(FILE *f, const struct ast *a)
{
	assert(f != NULL);

	if (a == NULL) {
		return 0;
	}

	switch (a->type) {
	case AST_STR:   return !fprintf(f, "%s", a->u.s);
	case AST_EXEC:  return dump_list(f, "",  "",  a->u.l);
	case AST_LIST:  return dump_list(f, "(", ")", a->u.l);

	case AST_DEREF: return dump_block(f, "$",  "",   a);
	case AST_BLOCK: return dump_block(f, "{ ", " }", a);
	case AST_CALL:  return dump_block(f, "(",  ")",  a);
	case AST_SETBG: return dump_block(f, "",   "&",  a);

	case AST_AND:    return dump_op(f, " && ", a);
	case AST_OR:     return dump_op(f, " || ", a);
	case AST_JOIN:   return dump_op(f, "^",    a);
	case AST_PIPE:   return dump_op(f, " | ",  a);
	case AST_ASSIGN: return dump_op(f, "=",    a);
	case AST_SEP:    return dump_op(f, "; ",   a);

	default:
		fprintf(f, "? %d ", a->type);
		return 0;
	}
}

int
out_qs(FILE *f, const struct ast *a)
{
	assert(f != NULL);

	dump_node(f, a);

	return 0;
}

