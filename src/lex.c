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
	case '.':
	case '`':
	case '{':
	case '}':
	case '(':
	case ')':
		*s = *p;
		*e = *s + 1;
		(*p)++;
		return **s;

	case '\'':
		(*p)++;
		*s = *p;
		*p += strcspn(*p, "\'");
		*e = *p - 1;
		(*p) += **p == '\'';
		return tok_str;

	case '$':
		(*p)++;
		*s = *p;
		*p += !!strcspn(*p, WHITE);
		*p += strcspn(*p, WHITE "&^|;=.`$'#{}");
		*e = *p;
		return tok_var;

	default:
		*s = *p;
		*p += strcspn(*p, WHITE "&^|;=.`$'#{}");
		*e = *p;
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

	return tok_str;
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

		if (!fgets(l->buf, sizeof l->buf, l->f)) {
			if (errno != 0) {
				perror("fgets");

				t->type = tok_error;

				return;
			}

			t->type = tok_eof;

			return;
		}

		if (debug & DEBUG_BUF) {
			fprintf(stderr, "[%s]\n", l->buf);
		}

		if (l->buf[sizeof l->buf - 1] == '\0' && l->buf[sizeof l->buf - 2] != '\n') {
			int c;

			fprintf(stderr, "underflow; panic\n");

			while (c = fgetc(l->f), c != EOF && c != '\n')
				;

			t->type = tok_panic;

			return;
		}

		l->p = l->buf;
	}

	t->type = lex_push(&l->p, &t->s, &t->e);

	if (debug & DEBUG_LEX) {
		const char *name;

		switch (t->type) {
		case tok_eof: name = "eof "; break;
		case tok_nl:  name = "nl ";  break;
		case tok_str: name = "str "; break;
		case tok_var: name = "var "; break;
		default:      name = "";     break;
		}

		fprintf(stderr, "<%s\"%.*s\">\n",
			name, (int) (t->e - t->s), t->s);
	}

	return;
}

