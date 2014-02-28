#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "../ast.h"
#include "../var.h"
#include "../frame.h"

#include "../out.h"

int
dump_frame(const struct frame *f)
{
	assert(f != NULL);

	{
		const struct var *p;

		fprintf(stderr, "\t\"%p\" [ shape = record, color = white, label = <",
			(void *) f);
		fprintf(stderr, "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" COLOR=\"red\">");

		for (p = f->var; p != NULL; p = p->next) {
			fprintf(stderr, "<TR>");

			fprintf(stderr, "<TD>$%s</TD>", p->name);

			/* TODO: escape " and | for dot */
			/* TODO: trim if it's too long */
			fprintf(stderr, "<TD ALIGN=\"LEFT\">");
			out_qs(p->a);
			fprintf(stderr, "</TD>");

			fprintf(stderr, "</TR>");
		}

		fprintf(stderr, "</TABLE>> ];\n");
	}

	if (f->parent != NULL) {
		fprintf(stderr, "\t\"%p\" -- \"%p\" [ dir = back, color = red, constraint=false ];\n",
			(void *) f->parent, (void *) f);
	}

	return 0;
}

static int
dump_node(const struct ast *a)
{
	if (a == NULL) {
		return 0;
	}

	switch (a->type) {
    case AST_STR:
    case AST_EXEC:
    case AST_LIST:
        return 0;

    case AST_BLOCK:
		dump_frame(a->f);
    case AST_DEREF:
    case AST_CALL:
    case AST_SETBG:
		dump_node(a->u.a);
		return 0;

    case AST_AND:
    case AST_OR:
    case AST_JOIN:
    case AST_PIPE:
    case AST_ASSIGN:
    case AST_SEP:
		dump_node(a->u.op.a);
		dump_node(a->u.op.b);
		return 0;

	default:
		return 0;
	}
}

int
out_frame(const struct ast *a)
{
	fprintf(stderr, "graph G {\n");
	fprintf(stderr, "\tnode [ shape = plaintext ];\n");
	fprintf(stderr, "\tsplines = line;\n");

	dump_node(a);

	fprintf(stderr, "}\n");

	return 0;
}

