/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ERR_H
#define ERR_H

void
err_printf(const char *class,
	const char *buf, const struct lex_pos *pos,
	const char *fmt, ...);

#endif

