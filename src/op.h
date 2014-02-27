#ifndef OP_H
#define OP_H

int op_and   (struct ast *a, struct ast *b);
int op_or    (struct ast *a, struct ast *b);
int op_join  (struct ast *a, struct ast *b);
int op_pipe  (struct ast *a, struct ast *b);
int op_assign(struct ast *a, struct ast *b);
int op_sep   (struct ast *a, struct ast *b);

#endif

