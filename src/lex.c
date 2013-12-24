#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define WHITE " \t\v\f\n"

enum lex_tok {
	tok_eol,
	tok_sep,
	tok_pipe,
	tok_str,
	tok_word,
};

struct lex_state {
	char c;
};

enum lex_tok
lex_push(const char **p, const char **s, const char **e)
{
	assert(p != NULL);

	*p += strspn(*p, WHITE);

	switch (**p) {
	case '\0': *s = *p; *e = *s;             return tok_eol;
	case ';':  *s = *p; *e = *s + 1; (*p)++; return tok_sep;
	case '|':  *s = *p; *e = *s + 1; (*p)++; return tok_pipe;

	case '\'':
		(*p)++;
		*s = *p;
		*p += strcspn(*p, "\'");
		*e = *(p - 1);
		(*p) += **p == '\'';
		return tok_str;

	default:
		*s = *p;
		*p += strcspn(*p, WHITE ";|'");
		*e = *p;
		return tok_word;
	}
}

int
main(void)
{
	char buf[4096];

	for (;;) {
		const char *p;

		buf[sizeof buf - 1] = 'x';

		if (!fgets(buf, sizeof buf, stdin)) {
			break;
		}

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			int c;

			fprintf(stderr, "underflow; panic\n");

			while (c = fgetc(stdin), c != EOF && c != '\n')
				;

			continue;
		}

		{
			enum lex_tok t;

			p = buf;

			do {
				const char *s, *e;

				t = lex_push(&p, &s, &e);

				printf("<#%d '%.*s'>\n", t, (int) (e - s), s);
			} while (t != tok_eol);
		}
	}

	return 0;
}

