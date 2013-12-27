#ifndef LEX_H
#define LEX_H

enum lex_tok {
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

int
lex_next(const char **s, const char **e);

#endif

