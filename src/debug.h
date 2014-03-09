#ifndef MAIN_H
#define MAIN_H

enum {
	DEBUG_BUF   = 1 << 0,
	DEBUG_LEX   = 1 << 1,
	DEBUG_PARSE = 1 << 2,
	DEBUG_ACT   = 1 << 3,
	DEBUG_FRAME = 1 << 4,
	DEBUG_STACK = 1 << 5,
	DEBUG_EXEC  = 1 << 6
};

extern unsigned debug;

#endif

