#ifndef LIST_H
#define LIST_H

#include "list.h"

struct ast;

struct ast_list *
list_cat(struct ast_list *l, struct ast *a);

#endif

