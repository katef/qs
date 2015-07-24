#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "code.h"

const char *
code_name(enum code_type type)
{
	switch (type) {
	case CODE_CALL: return "call";
	case CODE_JOIN: return "join";
	case CODE_NOT:  return "not";
	case CODE_NULL: return "null";
	case CODE_RUN:  return "run";
	case CODE_TICK: return "tick";
	case CODE_DUP:  return "dup";
	case CODE_ASC:  return "asc";
	case CODE_PUSH: return "push";
	case CODE_POP:  return "pop";
	case CODE_SIDE: return "side";

	case CODE_DATA: return "data";
	case CODE_IF:   return "if";
	case CODE_PIPE: return "pipe";
	case CODE_SET:  return "set";
	}

	return "?";
}

struct code *
code_anon(struct code **head,
	enum code_type type, struct code *code)
{
	struct code *new;

	assert(head != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = type;
	new->u.code = code;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", code_name(new->type));
		/* TODO: could dump code here */
	}

	return new;
}

struct code *
code_data(struct code **head,
	size_t n, const char *s)
{
	struct code *new;

	assert(head != NULL);
	assert(s != NULL);

	new = malloc(sizeof *new + n + 1);
	if (new == NULL) {
		return NULL;
	}

	new->type   = CODE_DATA;
	new->u.s    = memcpy((char *) new + sizeof *new, s, n);
	new->u.s[n] = '\0';

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s \"%.*s\"\n", code_name(new->type), (int) n, s);
	}

	return new;
}

struct code *
code_push(struct code **head,
	enum code_type type)
{
	struct code *new;

	assert(head != NULL);
	assert(type & CODE_NONE);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type  = type;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", code_name(new->type));
	}

	return new;
}

static struct code *
code_clone(struct code **head, const struct code *code)
{
	switch (code->type) {
	case CODE_DATA: return code_data(head, strlen(code->u.s), code->u.s);
	case CODE_IF:   return code_anon(head, code->type, code->u.code);
	case CODE_PIPE: return code_anon(head, code->type, code->u.code);
	case CODE_SET:  return code_anon(head, code->type, code->u.code);
	default:        return code_push(head, code->type);
	}

	errno = EINVAL;
	return NULL;
}

int
code_wind(struct code **head, const struct code *code)
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

	assert((*q)->next == NULL);
	(*q)->next = *head;

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
		fprintf(stderr, "code <- %s\n", code_name(node->type));
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
		fprintf(f, "#%s", code_name(p->type));

		switch (p->type) {
		case CODE_DATA:
			fprintf(f, "'%s'", p->u.s);
			break;

		case CODE_IF:
		case CODE_SET:
		case CODE_PIPE:
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

