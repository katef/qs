#ifndef EXEC_H
#define EXEC_H

int
count_args(const struct data *data);

char **
make_args(const struct data *data, int n);

int
exec_cmd(struct frame *f, int argc, char **argv);

#endif

