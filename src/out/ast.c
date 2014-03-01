#include <assert.h>
#include <stdio.h>

#include "../ast.h"
#include "../var.h"
#include "../debug.h"
#include "../out.h"

int
dump_frame(FILE *f, const struct frame *fr);

static int
dump_node(FILE *f, const struct ast *a, const void *node);

static int
dump_list(FILE *f, const char *op, const struct ast_list *l, const void *node)
{
	const struct ast_list *p;

	assert(f != NULL);

	fprintf(f, "\t\"%p\" [ shape = record, style = rounded, label = \"{<op>%s|{", node, op);

	for (p = l; p != NULL; p = p->next) {
		fprintf(f, "<%p>", (void *) p);

		switch (p->a->type) {
		case AST_STR:
			fprintf(f, "%s", p->a->u.s);
			break;

		default:
			fprintf(f, "?");
			break;
		}

		if (p->next) {
			fprintf(f, "|");
		}
	}

	fprintf(f, "}}\" ];\n");

	return 0;
}

static int
dump_block(FILE *f, const char *op, const struct ast *a, const void *node)
{
	assert(f != NULL);
	assert(op != NULL);
	assert(a != NULL);
	assert(a->f != NULL);

	fprintf(f, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(f, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.a,
		a->u.a == NULL ? "invis" : "solid");

	if (debug & DEBUG_FRAME) {
		fprintf(f, "\t{ \"%p\"; \"%p\"; rank = same; };\n", node, (void *) a->f);

		fprintf(f, "\t\"%p\" -- \"%p\" [ style = dotted ];\n",
			node, (void *) a->f);

		dump_frame(f, a->f);
	}

	dump_node(f, a->u.a, &a->u.a);

	return 0;
}

static int
dump_op(FILE *f, const char *op, const struct ast *a, const void *node)
{
	assert(f != NULL);
	assert(op != NULL);
	assert(a != NULL);

	fprintf(f, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(f, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.op.a,
		a->u.op.a == NULL ? "invis" : "solid");

	fprintf(f, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.op.b,
		a->u.op.b == NULL ? "invis" : "solid");

	dump_node(f, a->u.op.a, &a->u.op.a);
	dump_node(f, a->u.op.b, &a->u.op.b);

	return 0;
}

static int
dump_node(FILE *f, const struct ast *a, const void *node)
{
	assert(f != NULL);

	if (a == NULL) {
		fprintf(f, "\t\"%p\" [ style = invis ];\n", node);
		return 0;
	}

	switch (a->type) {
	case AST_STR:
		fprintf(f, "\t\"%p\" [ label = \"'%s'\" ];\n", node, a->u.s);
		return 0;

	case AST_EXEC:
		return dump_list(f, "!", a->u.l, node);

	case AST_LIST:  return dump_list(f, "( )", a->u.l, node);

	case AST_DEREF: return dump_block(f, "$",   a, node);
	case AST_BLOCK: return dump_block(f, "{ }", a, node);
	case AST_CALL:  return dump_block(f, "()",  a, node);
	case AST_SETBG: return dump_block(f, "bg",  a, node);

	case AST_AND:    return dump_op(f, "&&", a, node);
	case AST_OR:     return dump_op(f, "||", a, node);
	case AST_JOIN:   return dump_op(f, "^",  a, node);
	case AST_PIPE:   return dump_op(f, "|",  a, node);
	case AST_ASSIGN: return dump_op(f, "=",  a, node);
	case AST_SEP:    return dump_op(f, ";",  a, node);

	default:
		fprintf(f, "\t\"%p\" [ shape = box, style = rounded, label = \"%s %d\" ];\n",
			node, "?", a->type);
		return 0;
	}
}

int
out_ast(FILE *f, const struct ast *a)
{
	assert(f != NULL);

	fprintf(f, "graph G {\n");
	fprintf(f, "\tnode [ shape = plaintext ];\n");
	fprintf(f, "\tsplines = line;\n");

	dump_node(f, a, &a);

	fprintf(f, "}\n");

	return 0;
}

