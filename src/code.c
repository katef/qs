#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "code.h"

const char *
code_name(enum code_type type)
{
	switch (type) {
	case CODE_NULL: return "null";
	case CODE_ANON: return "anon";
	case CODE_DATA: return "data";
	case CODE_NOT:  return "not";
	case CODE_CALL: return "call";
	case CODE_EXEC: return "exec";
	case CODE_IF:   return "if";
	case CODE_JOIN: return "join";
	case CODE_PIPE: return "pipe";
	case CODE_SET:  return "set";
	}

	return "?";
}

struct code *
code_anon(struct code **head, struct code *code)
{
	struct code *new;

	assert(head != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type   = CODE_ANON;
	new->u.code = code;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", code_name(new->type));
	}

	return new;
}

struct code *
code_data(struct code **head, size_t n, const char *s)
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
code_push(struct code **head, enum code_type type, struct frame *frame)
{
	struct code *new;

	assert(head != NULL);
	assert(frame != NULL);
	assert(type != CODE_ANON);
	assert(type != CODE_DATA);
	assert(type && !(type & (type - 1)));

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->type    = type;
	new->u.frame = frame;

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "code -> %s\n", code_name(new->type));
	}

	return new;
}

struct code *
code_pop(struct code **head)
{
	struct code *node;

	assert(head != NULL);

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

struct code **
code_clone(struct code **dst, const struct code *src)
{
	const struct code *p;
	struct code **q, *end;

	assert(dst != NULL);

	end = *dst;

	for (p = src, q = dst; p != NULL; p = p->next) {
		*q = malloc(sizeof **q);
		if (*q == NULL) {
			goto error;
		}

		if (debug & DEBUG_STACK) {
			fprintf(stderr, "code -> %s\n", code_name(p->type));
		}

		/* note .u still points into src */
		(*q)->type    = p->type;
		if (p->type == CODE_DATA) {
			(*q)->u.s     = p->u.s;
		} else {
			(*q)->u.frame = p->u.frame;
		}

		q = &(*q)->next;
	}

	*q = end;

	return q;

error:

	code_free(*dst);
	*dst = end;

	return NULL;
}

static int
code_dumpinline(FILE *f, const struct code *code)
{
	const struct code *p;

	assert(f != NULL);

	for (p = code; p != NULL; p = p->next) {
		switch (p->type) {
		case CODE_DATA:
			fprintf(f, "'%s' ", p->u.s);
			break;

		case CODE_ANON:
			fprintf(f, "{ ");
			code_dumpinline(f, p->u.code);
			fprintf(f, "} ");
			break;

		default:
			fprintf(f, "#%s ", code_name(p->type));
		}
	}

	return 0;
}

int
code_dump(FILE *f, const struct code *code)
{
	assert(f != NULL);

	code_dumpinline(f, code);

	fprintf(f, "\n");

	return 0;
}

