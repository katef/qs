#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"
#include "var.h"
#include "frame.h"

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
	new->f      = NULL;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	return new;
}

struct ast *
ast_new_list(struct ast_list *l)
{
	struct ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type = AST_LIST;
	new->f    = NULL;
	new->u.l  = l;

	return new;
}

struct ast *
ast_new_exec(struct frame *f, enum ast_type type, struct ast_list *l)
{
	struct ast *new;

	assert(f != NULL);

	if (l == NULL) {
		errno = EINVAL;
		return NULL;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type = type;
	new->f    = f;
	new->u.l  = l;

	return new;
}

struct ast *
ast_new_block(struct frame *f, enum ast_type type, struct ast *a)
{
	struct ast *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type  = type;
	new->f     = f;
	new->u.a   = a;

	return new;
}

struct ast *
ast_new_op(struct frame *f, enum ast_type type, struct ast *a, struct ast *b)
{
	struct ast *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->f      = f;
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
		struct frame *f;

	case AST_STR:
		break;

	case AST_BLOCK:
	case AST_DEREF:
	case AST_CALL:
		f = frame_pop(&a->f);
		var_free(f->var);
		free(f);
		ast_free(a->u.a);
		break;

	default:
		ast_free(a->u.op.a);
		ast_free(a->u.op.b);
		break;
	}

	free(a);
}

static int
dump_frame(const struct frame *f, const void *node)
{
	assert(f != NULL);

	{
		const struct var *p;

		fprintf(stderr, "\t\"%p\" [ shape = record, color = red, label = \"{",
			(void *) f);

		for (p = f->var; p != NULL; p = p->next) {
			fprintf(stderr, "$%s", p->name);

			if (p->next != NULL) {
				fprintf(stderr, "|");
			}
		}

		fprintf(stderr, "}\" ];\n");
	}

	fprintf(stderr, "\t{ \"%p\"; \"%p\"; rank = same; };\n", node, (void *) f);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = dotted ];\n",
		node, (void *) f);

	if (f->parent != NULL) {
		fprintf(stderr, "\t\"%p\" -- \"%p\" [ dir = back, color = red, constraint=false ];\n",
			(void *) f->parent, (void *) f);
	}

	return 0;
}

static int
dump_list(const char *op, const struct ast_list *l, const void *node)
{
	const struct ast_list *p;

	fprintf(stderr, "\t\"%p\" [ shape = record, style = rounded, label = \"{<op>%s|{", node, op);

	for (p = l; p != NULL; p = p->next) {
		fprintf(stderr, "<%p>", (void *) p);

		switch (p->a->type) {
		case AST_STR:
			fprintf(stderr, "%s", p->a->u.s);
			break;

		default:
			fprintf(stderr, "?");
			break;
		}

		if (p->next) {
			fprintf(stderr, "|");
		}
	}

	fprintf(stderr, "}}\" ];\n");

	return 0;
}

static int
dump_block(const char *op, const struct ast *a, const void *node)
{
	assert(op != NULL);
	assert(a != NULL);
	assert(a->f != NULL);

	fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &a->u.a,
		a->u.a == NULL ? "invis" : "solid");

	dump_frame(a->f, node);
	dump_node(a->u.a, &a->u.a);

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

	case AST_EXEC:
		fprintf(stderr, "\t\"%p\" -- \"%p\":op:c [ dir = back, color = red, constraint=false ];\n",
			(void *) a->f, node);
		return dump_list("!", a->u.l, node);

	case AST_LIST:  return dump_list("( )", a->u.l, node);

	case AST_DEREF: return dump_block("$",   a, node);
	case AST_BLOCK: return dump_block("{ }", a, node);
	case AST_CALL:  return dump_block("()",  a, node);
	case AST_SETBG: return dump_block("bg",  a, node);

	case AST_AND:    return dump_op("&&", a, node);
	case AST_OR:     return dump_op("||", a, node);
	case AST_JOIN:   return dump_op("^",  a, node);
	case AST_PIPE:   return dump_op("|",  a, node);
	case AST_ASSIGN: return dump_op("=",  a, node);
	case AST_SEP:    return dump_op(";",  a, node);

	default:
		fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s %d\" ];\n",
			node, "?", a->type);
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

