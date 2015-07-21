#ifndef ARGS_H
#define ARGS_H

int
count_args(const struct data *data);

char **
make_args(const struct data *data, int n);

int
set_args(struct frame *frame, char *args[]);

void
dump_args(const char *name, char *const args[]);

#endif

