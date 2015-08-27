#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "lex.h"
#include "data.h"

struct data *
data_push(struct data **head, const struct lex_mark *mark,
	size_t n, const char *s)
{
	struct data *new;

	assert(head != NULL);
	assert(mark != NULL);
	assert(s != NULL || n == 0);

	new = malloc(sizeof *new + n + !!s);
	if (new == NULL) {
		return NULL;
	}

	new->mark = lex_mark(mark->buf, mark->pos);
	if (new->mark == NULL) {
		free(new);
		return NULL;
	}

	if (s != NULL) {
		new->s    = memcpy((char *) new + sizeof *new, s, n);
		new->s[n] = '\0';
	} else {
		new->s    = NULL;
	}

	new->next = *head;
	*head = new;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data -> %s\n", new->s ? new->s : "NULL");
	}

	return new;
}

struct data *
data_int(struct data **head, const struct lex_mark *mark,
	int n)
{
	char s[32]; /* XXX */
	int r;

	assert(head != NULL);
	assert(mark != NULL);

	r = sprintf(s, "%d", n);
	if (r == -1) {
		return NULL;
	}

	return data_push(head, mark, r, s);
}

struct data *
data_pop(struct data **head)
{
	struct data *node;

	assert(head != NULL);
	assert(*head != NULL);

	node = *head;
	*head = node->next;
	node->next = NULL;

	if (debug & DEBUG_STACK) {
		fprintf(stderr, "data <- %s\n", node->s ? node->s : "NULL");
	}

	return node;
}

void
data_free(struct data *data)
{
	struct data *p, *next;

	for (p = data; p != NULL; p = next) {
		next = p->next;

		free(p->mark);
		free(p);
	}
}

void
data_dump(FILE *f, const struct data *data)
{
	const struct data *p;

	assert(f != NULL);

	for (p = data; p != NULL; p = p->next) {
		if (p->s == NULL) {
			fprintf(stderr, "NULL ");
		} else {
			fprintf(stderr, "'%s' ", p->s);
		}
	}

	fprintf(stderr, "\n");
}

