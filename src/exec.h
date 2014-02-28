#ifndef EXEC_H
#define EXEC_H

struct frame;

int
exec_cmd(struct frame *f, int argc, char **argv);

#endif

