#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "lex.h"
#include "err.h"

void
err_printf(const char *class,
	const char *buf, const struct lex_pos *pos,
	const char *fmt, ...)
{
	assert(buf != NULL);
	assert(pos != NULL);

	if (class != NULL) {
		fprintf(stderr, "%s ", class);
	}

	fprintf(stderr, "error: ");

	if (fmt != NULL) {
		va_list ap;

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}

	fprintf(stderr, "\n");

	/*
	 * The lexer's buffer is a line at a time. Here we assume that the
	 * start of the buffer is always the start of a line, and so pos.col
	 * gives the start of the unexpected token.
	 *
	 * For a block-at-a-time lexer, I would prefix "..." if the buffer
	 * were not at the start of a line.
	 */

	/* TODO: conditionally on DEBUG_SRC (default to true) and buf != NULL */
	/* TODO: also position conditionally on pos != NULL */
	{
		size_t i;
		int r;

		r = fprintf(stderr, "at %lu:%lu: ", pos->line, pos->col);
		if (r < 0) {
			perror("fprintf");
			return;
		}

		fprintf(stderr, "%.*s\n", (int) strcspn(buf, "\n"), buf);

		for (i = 0; (int) i < r; i++) {
			fprintf(stderr, "-");
		}

		for (i = 1; i < pos->col; i++) {
			fprintf(stderr, "-");
		}

		fprintf(stderr, "^\n");
	}
}

