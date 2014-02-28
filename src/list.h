#ifndef LIST_H
#define LIST_H

#include "list.h"

struct ast;

struct ast_list *
list_cat(struct ast_list *l, struct ast *a);

struct ast_list *
list_args(int argc, char *argv[]);

#endif

