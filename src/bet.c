#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bet.h"

struct bet *
bet_new_leaf(enum bet_type type, size_t n, const char *s)
{
	struct bet *new;

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

struct bet *
bet_new_branch(enum bet_type type, struct bet *a, struct bet *b)
{
	struct bet *new;

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
bet_free(struct bet *bet)
{
	if (bet == NULL) {
		return;
	}

	switch (bet->type) {
	case BET_STR:
	case BET_VAR:
		break;

	default:
		bet_free(bet->u.op.a);
		bet_free(bet->u.op.b);
		break;
	}

	free(bet);
}

static int
dump_node(const struct bet *bet, const void *node)
{
	const char *op;

	if (bet == NULL) {
		fprintf(stderr, "\t\"%p\" [ style = invis ];\n", node);
		return 0;
	}

	switch (bet->type) {
	case BET_STR: fprintf(stderr, "\t\"%p\" [ label = \"'%s'\" ];\n", node, bet->u.s); return 0;
	case BET_VAR: fprintf(stderr, "\t\"%p\" [ label = \"$%s\"  ];\n", node, bet->u.s); return 0;

	case BET_AND:    op = "&&"; break;
	case BET_OR:     op = "||"; break;
	case BET_JOIN:   op = "^";  break;
	case BET_PIPE:   op = "|";  break;
	case BET_ASSIGN: op = "=";  break;
	case BET_EXEC:   op = ";";  break;
	case BET_BG:     op = "&";  break;
	case BET_CONS:   op = ",";  break;

	default:
		op = "?";
		break;
	}

	fprintf(stderr, "\t\"%p\" [ shape = box, style = rounded, label = \"%s\" ];\n", node, op);

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &bet->u.op.a,
		bet->u.op.a == NULL ? "invis" : "solid");

	fprintf(stderr, "\t\"%p\" -- \"%p\" [ style = %s ];\n",
		node, (void *) &bet->u.op.b,
		bet->u.op.b == NULL ? "invis" : "solid");

	dump_node(bet->u.op.a, &bet->u.op.a);
	dump_node(bet->u.op.b, &bet->u.op.b);

	return 0;
}

int
bet_dump(const struct bet *bet)
{
	fprintf(stderr, "graph G {\n");
	fprintf(stderr, "\tnode [ shape = plaintext ];\n");

	dump_node(bet, &bet);

	fprintf(stderr, "}\n");

	return 0;
}

