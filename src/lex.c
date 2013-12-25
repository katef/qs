#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define WHITE " \t\v\f\n"

enum lex_tok {
	tok_eol,
	tok_sep,
	tok_pipe,
	tok_str,
	tok_word
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

enum lex_tok
lex_next(const char **s, const char **e)
{
	static char buf[4096];
	static const char *p = buf;

	if (*p == '\0') {
		buf[sizeof buf - 1] = 'x';
		errno = 0;

		if (!fgets(buf, sizeof buf, stdin)) {
			if (errno != 0) {
				perror("fgets");
				return -1;
			}

			return tok_eol;
		}

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			int c;

			fprintf(stderr, "underflow; panic\n");

			while (c = fgetc(stdin), c != EOF && c != '\n')
				;

			return -1;
		}
	}

	return lex_push(&p, s, e);
}

static struct ast *
parse(void)
{
	const char *s, *e;
	struct ast *ast;
	enum lex_tok t;

	do {
		/* TODO: recursive descent parser here */
		t = lex_next(&s, &e);
		if (t == -1) {
/* TODO: free ast in progress */
			return NULL;
		}

		printf("<#%d '%.*s'>\n", t, (int) (e - s), s);
	} while (t != tok_eol);

	ast = NULL;

	return ast;
}

int
main(void)
{
	struct ast *ast;

	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	for (;;) {
		ast = parse();
		if (ast == NULL) {
			perror("parse");
			return -1;
		}

		/* TODO: do something with ast */
	}

	return 0;
}

