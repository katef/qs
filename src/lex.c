/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"

#define WHITE " \t\v\f\n"

static enum lex_type
lex_push(const char **p, const char **s, const char **e)
{
#if 0
	size_t i;

	struct {
		enum lex_type type;
		const char *name;
	} keywords[] = {
		{ tok_if,    "if"    },
		{ tok_else,  "else"  },
		{ tok_while, "while" }
	};
#endif

	assert(p != NULL);

	*p += strspn(*p, WHITE);

	switch (**p) {
	case '#':
		*p += strcspn(*p, "\n");
	case '\0':
		*s = *p;
		*e = *s;
		return tok_nl;

	case '&':
		if (*(*p + 1) == '&') {
			*s = *p;
			*e = *s + 2;
			(*p) += 2;
			return tok_and;
		}

	case '|':
		if (*(*p + 1) == '|') {
			*s = *p;
			*e = *s + 2;
			(*p) += 2;
			return tok_or;
		}

	case '^':
	case ';':
	case '=':
	case ',':
	case '`':
	case '{': case '}':
	case '(': case ')':
	case '[': case ']':
		*s = *p;
		*e = *s + 1;
		(*p)++;
		return **s;

	case '\'':
		(*p)++;
		*s = *p;
		*p += strcspn(*p, "\'");
		*e = *p;
		(*p) += **p == '\'';
		return tok_word;

	case '$':
		(*p)++;
		*s = *p;
		*p += !!strcspn(*p, WHITE);
		*p += strcspn(*p, WHITE "&^|;=`$'#{}(),");
		*e = *p;
		return tok_var;

	default:
		*s = *p;
		*p += strcspn(*p, WHITE "&^|;=`$'#{}()[],"); /* XXX: , in [] zone only */
		*e = *p;
		break;
	}

	if (**s == '.' && *e == *s + 1) {
		return tok_dot;
	}

#if 0
	for (i = 0; i < sizeof keywords / sizeof *keywords; i++) {
		if (*e - *s != (int) strlen(keywords[i].name)) {
			continue;
		}

		if (0 == strncmp(*s, keywords[i].name, *e - *s)) {
			return keywords[i].type;
		}
	}
#endif

	return tok_word;
}

void
lex_next(struct lex_state *l, struct lex_tok *t)
{
	assert(l != NULL);
	assert(l->f != NULL);
	assert(l->p != NULL);
	assert(t != NULL);

	if (*l->p == '\0') {
		l->buf[sizeof l->buf - 1] = 'x';
		errno = 0;

		l->pos.col = 1;
		l->pos.line++;

		if (!fgets(l->buf, sizeof l->buf, l->f)) {
			if (errno != 0) {
				perror("fgets");

				t->type = tok_error;
				t->pos  = l->pos;

				return;
			}

			t->type = tok_eof;
			t->pos  = l->pos;

			return;
		}

		if (debug & DEBUG_BUF) {
			fprintf(stderr, "%lu:%lu [%s]\n", l->pos.line, l->pos.col, l->buf);
		}

		if (l->buf[sizeof l->buf - 1] == '\0' && l->buf[sizeof l->buf - 2] != '\n') {
			int c;

			fprintf(stderr, "underflow; panic\n");

			if (debug & DEBUG_LEX) {
				fprintf(stderr, "<panic %lu:%lu .. %lu:%lu>\n",
					l->pos.line, l->pos.col,
					l->pos.line + 1, 1UL);
			}

			while (c = fgetc(l->f), c != EOF && c != '\n')
				;

			/* TODO: handle EOF error; lex_panic first, then lex_eof */

			l->pos.col = 1;
			l->pos.line++;

			t->type = tok_panic;
			t->pos  = l->pos;

			return;
		}

		l->p = l->buf;
	}

	t->type = lex_push(&l->p, &t->s, &t->e);

	l->pos.col = t->s - l->buf + 1;

	t->pos  = l->pos;

	if (debug & DEBUG_LEX) {
		const char *name;

		switch (t->type) {
		case tok_eof:  name = "eof ";  break;
		case tok_nl:   name = "nl ";   break;
		case tok_word: name = "word "; break;
		case tok_var:  name = "var ";  break;
		default:       name = "";      break;
		}

		fprintf(stderr, "<%s%lu:%lu \"%.*s\">\n",
			name, t->pos.line, t->pos.col,
			(int) (t->e - t->s), t->s);
	}
}

struct lex_mark *
lex_mark(const char *buf, struct lex_pos pos)
{
	struct lex_mark *new;
	size_t z;

	assert(buf != NULL);

	z = strlen(buf);

	new = malloc(sizeof *new + z + 1);
	if (new == NULL) {
		return NULL;
	}

	new->buf = strcpy((char *) new + sizeof *new, buf);
	new->pos = pos;

	return new;
}

