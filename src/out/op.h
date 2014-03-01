#ifndef EVAL_OP_H
#define EVAL_OP_H

int eval_op_and (struct ast *a, struct ast *b);
int eval_op_or  (struct ast *a, struct ast *b);
int eval_op_join(struct ast *a, struct ast *b);
int eval_op_pipe(struct ast *a, struct ast *b);
int eval_op_sep (struct ast *a, struct ast *b);

#endif

