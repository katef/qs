#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "ast.h"
#include "scope.h"
#include "status.h"

int
status_set(struct scope *sc, int r)
{
	struct ast *a;
	char s[32];
	int n;

	assert(sc != NULL);

	n = sprintf(s, "%d", r);
	if (n == -1) {
		return -1;
	}

	a = ast_new_leaf(AST_STR, n, s);
	if (a == NULL) {
		return -1;
	}

	if (!scope_set(sc, "?", a)) {
		ast_free(a);
		return -1;
	}

	return 0;
}

int
status_get(struct scope *sc, int *r)
{
	struct ast *a;
	char *e;
	long l;

	assert(sc != NULL);
	assert(r != NULL);

	a = scope_get(sc, "?");
	if (a == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (a->type != AST_STR) {
		errno = EINVAL;
		return -1;
	}

	l = strtol(a->u.s, &e, 10);
	if ((l == LONG_MAX || l == LONG_MIN) && errno == ERANGE) {
		return -1;
	}

	if (l > INT_MAX || l < INT_MIN) {
		errno = ERANGE;
		return -1;
	}

	if (*a->u.s == '\0' || *e != '\0') {
		errno = EINVAL;
		return -1;
	}

	*r = (int) l;

	return 0;
}

