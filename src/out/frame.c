#define _XOPEN_SOURCE 500

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"
#include "../frame.h"

#include "../out.h"

#define TMPDIR "/tmp"

/* TODO: centralise */
static int
filter(FILE *r, FILE *w,
	int (*filter)(int, FILE *))
{
	int c;

	assert(r != NULL && w != NULL);
	assert(filter != NULL);

	while (c = fgetc(r), c != EOF) {
		if (-1 == filter(c, w)) {
			return -1;
		}
	}

	if (ferror(r)) {
		return -1;
	}

	return 0;
}

static int
quote_dot(int c, FILE *f)
{
	const char *br = "<BR ALIGN='LEFT'/>";

	switch (c) {
	case '<':  return fputs("&lt;",  f);
	case '>':  return fputs("&gt;",  f);
	case '&':  return fputs("&amp;", f);
	case '{':  return fputs("\\{",   f);
	case '}':  return fputs("\\}",   f);

	/* some formatting to fit things in a <TD> */
	case '|':  return fprintf(f, "%s \\|", br);
	case ';':  return fprintf(f, ";%s",    br);
	case '\n': return fprintf(f, "%s",     br);

	default:   return fputc(c, f);
	}

	return 0;
}

int
dump_frame(FILE *f, const struct frame *fr)
{
	assert(f != NULL);
	assert(fr != NULL);

	{
		const struct var *p;

		fprintf(f, "\t\"%p\" [ shape = record, color = white, label = <",
			(void *) fr);
		fprintf(f, "<TABLE BORDER='0' CELLBORDER='1' CELLSPACING='0' COLOR=\"red\">");

		for (p = fr->var; p != NULL; p = p->next) {
			fprintf(f, "<TR>");

			fprintf(f, "<TD VALIGN='MIDDLE'>$%s</TD>", p->name);

			fprintf(f, "<TD ALIGN='LEFT'>");

			/* TODO: centralise */
			if (p->a != NULL) {
				FILE *fp;

				fp = tmpfile();
				if (fp == NULL) {
					return -1;
				}

				out_qs(fp, p->a);
				rewind(fp);
				filter(fp, f, quote_dot);

				fclose(fp);
			}

			fprintf(f, "</TD>");

			fprintf(f, "</TR>");
		}

		fprintf(f, "</TABLE>> ];\n");
	}

	if (fr->parent != NULL) {
		fprintf(f, "\t\"%p\" -- \"%p\" [ dir = back, color = red, constraint=false ];\n",
			(void *) fr->parent, (void *) fr);
	}

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
    case AST_STR:
    case AST_EXEC:
    case AST_LIST:
        return 0;

    case AST_BLOCK:
		dump_frame(f, a->f);
    case AST_DEREF:
    case AST_CALL:
    case AST_SETBG:
		dump_node(f, a->u.a);
		return 0;

    case AST_AND:
    case AST_OR:
    case AST_JOIN:
    case AST_PIPE:
    case AST_ASSIGN:
    case AST_SEP:
		dump_node(f, a->u.op.a);
		dump_node(f, a->u.op.b);
		return 0;

	default:
		return 0;
	}
}

int
out_frame(FILE *f, struct ast *a)
{
	assert(f != NULL);
	assert(a != NULL);

	fprintf(f, "graph G {\n");
	fprintf(f, "\tsplines = line;\n");

	dump_node(f, a);

	fprintf(f, "}\n");

	return 0;
}

