/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef DEBUG_H
#define DEBUG_H

enum {
	DEBUG_BUF   = 1 <<  0,
	DEBUG_LEX   = 1 <<  1,
	DEBUG_PARSE = 1 <<  2,
	DEBUG_ACT   = 1 <<  3,
	DEBUG_FRAME = 1 <<  4,
	DEBUG_STACK = 1 <<  5,
	DEBUG_VAR   = 1 <<  6,
	DEBUG_EVAL  = 1 <<  7,
	DEBUG_EXEC  = 1 <<  8,
	DEBUG_PROC  = 1 <<  9,
	DEBUG_SIG   = 1 << 10,
	DEBUG_FD    = 1 << 11
};

extern unsigned debug;

#endif

