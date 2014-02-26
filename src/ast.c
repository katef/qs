#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ast.h"
#include "var.h"
#include "scope.h"

static int
dump_node(const struct ast *a, const void *node);

struct ast *
ast_new_leaf(enum ast_type type, size_t n, const char *s)
{
	struct ast *new;

	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	return new;
}

struct ast *
ast_new_block(enum ast_type type, struct scope *sc, struct ast *a)
{
	struct ast *new;

	assert(sc != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type       = type;
	new->u.block.sc = sc;
	new->u.block.a  = a;

	return new;
}

struct ast *
ast_new_op(enum ast_type type, struct ast *a, struct ast *b)
{
	struct ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->u.op.a = a;
	new->u.op.b = b;

	return new;
}

void
ast_free(struct ast *a)
{
	if (a == NULL) {
		return;
	}

	switch (a->type) {
		struct scope *sc;

	case AST_STR:
		break;

	case AST_BLOCK:
	case AST_DEREF:
	case AST_CALL:
		sc = scope_pop(&a->u.block.sc);
		var_free(sc->var);
		free(sc);
		ast_free(a->u.block.a);
		break;

	default:
		ast_free(a->u.op.a);
		ast_free(a->u.op.b);
		break;
	}

	free(a);
}

static int
dump_scope(const struct scope *sc, const void *node)
{
	assert(sc != NULL);

	{
		const struct var *p;

		fprintf(stderr, "\t\"%p\" [ shape = record, color = red, label = \"{",
			(void *) sc);

		for (p = sc->var; p != NULL; p = p->next) {
			fprintf(stderr, "%s", p->name);

			if (p->next != NULL) {
				fprintf(stderr, "|");
			}
		}

		fprintf(stderr, "}\" ];\n");
	}

	fprintf(stderr, "\t{ \"%p\"; \"%p\"; rank = same; };\n", node, (void *) sc);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = dotted ];\n",
		node, (void *) sc);

	if (sc->parent != NULL) {
		fprintf(stderr, "\t\"%p\" -- \"%p\" [ dir = back, color = red, constraint=false ];\n",
			(void *) sc->parent, (void *) sc);
	}

	return 0;
}

static int
dump_block(const char *op, const struct ast *a, const void *node)
{
	assert(op != NULL);
	assert(a != NULL);
	assert(a->u.block.sc != NULL);

	fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.block.a,
		a->u.block.a == NULL ? "invis" : "solid");

	dump_scope(a->u.block.sc, node);
	dump_node(a->u.block.a, &a->u.block.a);

	return 0;
}

static int
dump_op(const char *op, const struct ast *a, const void *node)
{
	assert(op != NULL);
	assert(a != NULL);

	fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.op.a,
		a->u.op.a == NULL ? "invis" : "solid");

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.op.b,
		a->u.op.b == NULL ? "invis" : "solid");

	dump_node(a->u.op.a, &a->u.op.a);
	dump_node(a->u.op.b, &a->u.op.b);

	return 0;
}

static int
dump_node(const struct ast *a, const void *node)
{
	if (a == NULL) {
		fprintf(stderr, "\t\"%p\" [ style = invis ];\n", node);
		return 0;
	}

	switch (a->type) {
	case AST_STR:
		fprintf(stderr, "\t\"%p\" [ label = \"'%s'\" ];\n", node, a->u.s);
		return 0;

	case AST_DEREF: return dump_block("$",   a, node);
	case AST_BLOCK: return dump_block("{ }", a, node);
	case AST_CALL:  return dump_block("()",  a, node);

	case AST_AND:    return dump_op("&&", a, node);
	case AST_OR:     return dump_op("||", a, node);
	case AST_JOIN:   return dump_op("^",  a, node);
	case AST_PIPE:   return dump_op("|",  a, node);
	case AST_ASSIGN: return dump_op("=",  a, node);
	case AST_EXEC:   return dump_op(";",  a, node);
	case AST_BG:     return dump_op("&",  a, node);
	case AST_CONS:   return dump_op(",",  a, node);

	default:
		fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, "?");
		return 0;
	}
}

int
ast_dump(const struct ast *a)
{
	fprintf(stderr, "graph G {\n");
	fprintf(stderr, "\tnode [ shape = plaintext ];\n");
	fprintf(stderr, "\tsplines = line;\n");

	dump_node(a, &a);

	fprintf(stderr, "}\n");

	return 0;
}

