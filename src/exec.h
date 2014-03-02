#ifndef EXEC_H
#define EXEC_H

char **
make_argv(const struct data *data, int *argc);

int
exec_cmd(struct frame *f, int argc, char **argv);

#endif

