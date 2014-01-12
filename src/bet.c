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
indent(unsigned n)
{
	while (n--) {
		fprintf(stderr, "\t");
	}

	return 0;
}

static int
dump_node(const struct bet *bet, unsigned n)
{
	const char *op;

	if (-1 == indent(n)) {
		return -1;
	}

	if (bet == NULL) {
		fprintf(stderr, "NULL\n");
		return 0;
	}

	switch (bet->type) {
	case BET_STR: fprintf(stderr, "'%s'\n", bet->u.s); return 0;
	case BET_VAR: fprintf(stderr, "$%s\n",  bet->u.s); return 0;

	case BET_AND:    op = "&&"; break;
	case BET_OR:     op = "||"; break;
	case BET_JOIN:   op = "^";  break;
	case BET_PIPE:   op = "|";  break;
	case BET_ASSIGN: op = "=";  break;
	case BET_EXEC:   op = ";";  break;
	case BET_BG:     op = "&";  break;
	case BET_LIST:   op = "->"; break;

	default:
		op = "?";
		break;
	}

	fprintf(stderr, "%s\n", op);

	dump_node(bet->u.op.a, n + 1);
	dump_node(bet->u.op.b, n + 1);

	return 0;
}

int
bet_dump(const struct bet *bet)
{
	return dump_node(bet, 0);
}

