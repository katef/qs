#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"

#include "../out.h"

static int
dump_node(FILE *f, const struct ast *a);

static int
hasspecial(const char *s)
{
	assert(s != NULL);

	return s[strcspn(s, "#\\'\"`.{}();^|&$ \t\v\f\r\n")] != '\0';
}

static int
dump_str(FILE *f, const char *s)
{
	assert(f != NULL);

	/* TODO: quote whitespace as &#10; or something for graphviz */
	if (hasspecial(s)) {
		const char *p;

		fputc('"', f);

		for (p = s; *p != '\0'; p++) {
			switch (*p) {
			case '$':  fputs("\\$",  f); break;
			case '\\': fputs("\\\\", f); break;
			case '\t': fputs("\\t",  f); break;
			case '\v': fputs("\\v",  f); break;
			case '\f': fputs("\\f",  f); break;
			case '\r': fputs("\\r",  f); break;
			case '\n': fputs("\\n",  f); break;
			default:   fputc(*p,     f); break;
			}
		}

		fputc('"', f);
	} else {
		fputs(s, f);
	}
	return 0;
}

static int
dump_list(FILE *f, const char *s, const char *e, const struct ast_list *l)
{
	const struct ast_list *p;

	assert(f != NULL);
	assert(s != NULL);
	assert(e != NULL);

	fprintf(f, "%s", s);

	for (p = l; p != NULL; p = p->next) {
		switch (p->a->type) {
		case AST_STR:
			dump_str(f, p->a->u.s);
			break;

		default:
			fprintf(f, "?");
			break;
		}

		if (p->next) {
			fprintf(f, " ");
		}
	}

	fprintf(f, "%s", e);

	return 0;
}

static int
dump_block(FILE *f, const char *s, const char *e, const struct ast *a)
{
	assert(f != NULL);
	assert(s != NULL);
	assert(e != NULL);
	assert(a != NULL);

	fprintf(f, "%s", s);
	dump_node(f, a->u.a);
	fprintf(f, "%s", e);

	return 0;
}

static int
dump_op(FILE *f, const char *op, const struct ast *a)
{
	assert(f != NULL);
	assert(op != NULL);
	assert(a != NULL);

	dump_node(f, a->u.op.a);
	fprintf(f, "%s", op);
	dump_node(f, a->u.op.b);

	return 0;
}

static int
dump_node(FILE *f, const struct ast *a)
{
	assert(f != NULL);

	if (a == NULL) {
		return 0;
	}

	switch (a->type) {
	case AST_STR:   return dump_str(f, a->u.s);
	case AST_EXEC:  return dump_list(f, "",  "",  a->u.l);
	case AST_LIST:  return dump_list(f, "(", ")", a->u.l);

	case AST_DEREF: return dump_block(f, "$",  "",   a);
	case AST_BLOCK: return dump_block(f, "{ ", " }", a);
	case AST_CALL:  return dump_block(f, "(",  ")",  a);
	case AST_SETBG: return dump_block(f, "",   "&",  a);

	case AST_AND:    return dump_op(f, " && ", a);
	case AST_OR:     return dump_op(f, " || ", a);
	case AST_JOIN:   return dump_op(f, "^",    a);
	case AST_PIPE:   return dump_op(f, " | ",  a);
	case AST_ASSIGN: return dump_op(f, "=",    a);
	case AST_SEP:    return dump_op(f, "; ",   a);

	default:
		fprintf(f, "? %d ", a->type);
		return 0;
	}
}

int
out_qs(FILE *f, struct ast *a)
{
	assert(f != NULL);
	assert(a != NULL);

	dump_node(f, a);
	fputc('\n', f);

	return 0;
}

