#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"

#define WHITE " \t\v\f\n"

static enum lex_tok
lex_push(const char **p, const char **s, const char **e)
{
#if 0
	size_t i;

	struct {
		enum lex_tok tok;
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

	case ';':
	case '=':
	case '.':
	case '`':
	case '$':
	case '|':
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
		*e = *(p - 1);
		(*p) += **p == '\'';
		return tok_str;

	default:
		*s = *p;
		*p += strcspn(*p, WHITE ";|'#");
		*e = *p;
	}

#if 0
	for (i = 0; i < sizeof keywords / sizeof *keywords; i++) {
		if (*e - *s != (int) strlen(keywords[i].name)) {
			continue;
		}

		if (0 == strncmp(*s, keywords[i].name, *e - *s)) {
			return keywords[i].tok;
		}
	}
#endif

	return tok_str;
}

int
lex_next(const char **s, const char **e)
{
	static char buf[4096];
	static const char *p = buf;
	const char *name;
	enum lex_tok t;

	if (*p == '\0') {
		buf[sizeof buf - 1] = 'x';
		errno = 0;

		if (!fgets(buf, sizeof buf, stdin)) {
			if (errno != 0) {
				perror("fgets");
				return -1;
			}

			return tok_eof;
		}

		if (debug & DEBUG_BUF) {
			fprintf(stderr, "[%s]\n", buf);
		}

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			int c;

			fprintf(stderr, "underflow; panic\n");

			while (c = fgetc(stdin), c != EOF && c != '\n')
				;

			return -1;
		}

		p = buf;
	}

	t = lex_push(&p, s, e);

	switch (t) {
	case tok_eof: name = "eof "; break;
	case tok_nl:  name = "nl ";  break;
	case tok_str: name = "str "; break;
	default:      name = "";     break;
	}

	if (debug & DEBUG_LEX) {
		fprintf(stderr, "<%s\"%.*s\">\n",
			name, (int) (*e - *s), *s);
	}

	return t;
}

