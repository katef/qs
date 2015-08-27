#ifndef LEX_H
#define LEX_H

enum lex_type {
	tok_error  = -1,
	tok_panic  = -2,

	tok_eof     = '\0',
	tok_nl      = '\n',
	tok_word    = '\'',
	tok_semi    = ';',
	tok_bg      = '&',
	tok_equ     = '=',
	tok_comma   = ',',
	tok_dot     = '.',
	tok_tick    = '`',
	tok_var     = '$',
	tok_join    = '^',
	tok_bar     = '|',
	tok_obrace  = '{', tok_cbrace  = '}',
	tok_oparen  = '(', tok_cparen  = ')',
	tok_osquare = '[', tok_csquare = ']',
	tok_or      = 'o',
	tok_and     = 'a',
	tok_exec    = 'e'
};

struct lex_pos {
	unsigned long line;
	unsigned long col;
};

struct lex_tok {
	enum lex_type type;
	struct lex_pos pos;
	const char *s;
	const char *e;
};

struct lex_state {
	char buf[4096];
	const char *p;
	FILE *f;
	struct lex_pos pos;
};

void
lex_next(struct lex_state *l, struct lex_tok *t);

#endif

