#ifndef MAIN_H
#define MAIN_H

enum {
	DEBUG_BUF   = 1 << 0,
	DEBUG_LEX   = 1 << 1,
	DEBUG_PARSE = 1 << 2,
	DEBUG_FRAME = 1 << 3,
	DEBUG_STACK = 1 << 4,
	DEBUG_EXEC  = 1 << 5
};

extern unsigned debug;

#endif

