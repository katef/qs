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

struct data **
data_clone(struct data **dst, const struct data *src)
{
	const struct data *p;
	struct data **q, *end;

	assert(dst != NULL);
	assert(*dst == NULL);

	end = *dst;

fprintf(stderr, "clone src: ");
data_dump(stderr, src);
	for (p = src, q = dst; p != NULL; p = p->next) {
		*q = malloc(sizeof **q);
		if (*q == NULL) {
			goto error;
		}

		if (debug & DEBUG_STACK) {
			fprintf(stderr, "data -> %s\n", p->s ? p->s : "NULL");
		}

		/* note .s still points into src */
		(*q)->s = p->s;

		q = &(*q)->next;
	}
fprintf(stderr, "clone dst: ");
data_dump(stderr, *dst);

	*q = end;

	return q;

error:

	data_free(*dst);
	*dst = end;

	return NULL;
}

int
data_dump(FILE *f, const struct data *data)
{
	const struct data *p;

	assert(f != NULL);

	for (p = data; p != NULL; p = p->next) {
		fprintf(stderr, "data:%s\n", p->s ? p->s : "NULL");
	}

	return 0;
}

