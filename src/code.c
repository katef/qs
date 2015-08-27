#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"
#include "code.h"

const char *
op_name(enum opcode op)
{
	switch (op) {
	case OP_CALL: return "call";
	case OP_JOIN: return "join";
	case OP_NOT:  return "not";
	case OP_NULL: return "null";
	case OP_RUN:  return "run";
	case OP_TICK: return "tick";
	case OP_DUP:  return "dup";
	case OP_ASC:  return "asc";
	case OP_PUSH: return "push";
	case OP_POP:  return "pop";
	case OP_CLHS: return "clhs";
	case OP_CRHS: return "crhs";
	case OP_CTCK: return "ctck";

	case OP_DATA: return "data";
	case OP_IF:   return "if";
	case OP_PIPE: return "pipe";
	case OP_SET:  return "set";
	}

	return "?";
}

struct code *
code_anon(struct code **head, const struct lex_pos *pos,
	enum opcode op, struct code *code)
{
	struct code *new;

	assert(head != NULL);
	assert(pos != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->pos    = *pos;
	new->op     = op;
	new->u.code = code;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", op_name(new->op));
		/* TODO: could dump code here */
	}

	return new;
}

struct code *
code_data(struct code **head, const struct lex_pos *pos,
	size_t n, const char *s)
{
	struct code *new;

	assert(head != NULL);
	assert(pos != NULL);
	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->pos    = *pos;
	new->op     = OP_DATA;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s \"%.*s\"\n", op_name(new->op), (int) n, s);
	}

	return new;
}

struct code *
code_push(struct code **head, const struct lex_pos *pos,
	enum opcode op)
{
	struct code *new;

	assert(head != NULL);
	assert(pos != NULL);
	assert(op & CODE_NONE);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->pos = *pos;
	new->op  = op;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", op_name(new->op));
	}

	return new;
}

static struct code *
code_clone(struct code **head, const struct code *code)
{
	switch (code->op) {
	case OP_DATA: return code_data(head, &code->pos, strlen(code->u.s), code->u.s);
	case OP_IF:   return code_anon(head, &code->pos, code->op, code->u.code);
	case OP_PIPE: return code_anon(head, &code->pos, code->op, code->u.code);
	case OP_SET:  return code_anon(head, &code->pos, code->op, code->u.code);
	default:      return code_push(head, &code->pos, code->op);
	}

	errno = EINVAL;
	return NULL;
}

int
code_wind(struct code **head,
	const struct code *code)
{
	const struct code *p;
	struct code *new, **q;

	assert(head != NULL);

	new = NULL;

	for (p = code, q = &new; p != NULL; p = p->next, q = &(*q)->next) {
		if (!code_clone(q, p)) {
			code_free(new);
			return -1;
		}
	}

	assert(*q == NULL);
	*q = *head;

	*head = new;

	return 0;
}

struct code *
code_pop(struct code **head)
{
	struct code *node;

	assert(head != NULL);
	assert(*head != NULL);

	node = *head;
	*head = node->next;
	node->next = NULL;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code <- %s\n", op_name(node->op));
	}

	return node;
}

void
code_free(struct code *code)
{
	struct code *p, *next;

	for (p = code; p != NULL; p = next) {
		next = p->next;

		free(p);
	}
}

static void
code_dumpinline(FILE *f, const struct code *code)
{
	const struct code *p;

	assert(f != NULL);

	for (p = code; p != NULL; p = p->next) {
		fprintf(f, "#%s/%lu:%lu", op_name(p->op), p->pos.line, p->pos.col);

		switch (p->op) {
		case OP_DATA:
			fprintf(f, "'%s'", p->u.s);
			break;

		case OP_IF:
		case OP_SET:
		case OP_PIPE:
			fprintf(f, "{ ");
			code_dumpinline(f, p->u.code);
			fprintf(f, " }");
			break;

		default:
			break;
		}

		if (p->next != NULL) {
			fprintf(f, " ");
		}
	}
}

void
code_dump(FILE *f, const struct code *code)
{
	assert(f != NULL);

	code_dumpinline(f, code);

	fprintf(f, "\n");
}

