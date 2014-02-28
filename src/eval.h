#ifndef EVAL_H
#define EVAL_H

struct ast;

int
eval_ast(struct ast *a, struct ast **out);

#endif

