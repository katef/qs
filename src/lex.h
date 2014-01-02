#ifndef LEX_H
#define LEX_H

enum lex_type {
	tok_error  = -1,
	tok_panic  = -2,

	tok_eof    = '\0',
	tok_nl     = '\n',
	tok_str    = '\'',
	tok_semi   = ';',
	tok_equ    = '=',
	tok_dot    = '.',
	tok_back   = '`',
	tok_var    = '$',
	tok_pipe   = '|',
	tok_obrace = '{',
	tok_cbrace = '}',
	tok_oparen = '(',
	tok_cparen = ')',
	tok_if     = 'i',
	tok_else   = 'e',
	tok_while  = 'l'
};

struct lex_tok {
	enum lex_type type;
	const char *s;
	const char *e;
};

void
lex_next(struct lex_tok *t);

#endif

