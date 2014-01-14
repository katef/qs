#ifndef PARSE_H
#define PARSE_H

int
parse(struct lex_state *l,
	int (*dispatch)(struct bet *));

#endif

