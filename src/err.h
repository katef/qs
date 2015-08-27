#ifndef ERR_H
#define ERR_H

void
err_printf(const char *class,
	const char *buf, const struct lex_pos *pos,
	const char *fmt, ...);

#endif

