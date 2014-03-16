#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "data.h"

struct data *
data_push(struct data **head, size_t n, const char *s)
{
	struct data *new;

	assert(head != NULL);
	assert(s != NULL || n == 0);

	new = malloc(sizeof *new + n + !!s);
	if (new == NULL) {
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
data_pop(struct data **head)
{
	struct data *node;

	assert(head != NULL);

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

		free(p);
	}
}

int
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

	return 0;
}

