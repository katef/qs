#ifndef EVAL_H
#define EVAL_H

int
dispatch_anon(struct frame *frame, char *args[]);

int
eval(const struct code *code, struct data **data);

#endif

