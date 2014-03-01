#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"
#include "../frame.h"

#include "../out.h"

int
dump_frame(FILE *f, const struct frame *fr)
{
	assert(f != NULL);
	assert(fr != NULL);

	{
		const struct var *p;

		fprintf(f, "\t\"%p\" [ shape = record, color = white, label = <",
			(void *) fr);
		fprintf(f, "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" COLOR=\"red\">");

		for (p = fr->var; p != NULL; p = p->next) {
			fprintf(f, "<TR>");

			fprintf(f, "<TD>$%s</TD>", p->name);

			/* TODO: escape " and | for dot */
			/* TODO: trim if it's too long */
			fprintf(f, "<TD ALIGN=\"LEFT\">");
			out_qs(f, p->a);
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
out_frame(FILE *f, const struct ast *a)
{
	assert(f != NULL);

	fprintf(f, "graph G {\n");
	fprintf(f, "\tsplines = line;\n");

	dump_node(f, a);

	fprintf(f, "}\n");

	return 0;
}

