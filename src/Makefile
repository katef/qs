SID ?= sid
CC  ?= gcc

CFLAGS += -ansi -pedantic
CFLAGS += -Wall -W
#CFLAGS += -Werror
CFLAGS += -O2 -g -ggdb

SID_CFLAGS += -s no-numeric-terminals -s no-terminals

OBJS += lex.o
OBJS += parser.o
OBJS += ast.o
OBJS += exec.o
OBJS += builtin.o
OBJS += main.o
OBJS += var.o
OBJS += frame.o
OBJS += list.o
OBJS += code.o
OBJS += data.o

OBJS += out/qs.o
OBJS += out/ast.o
OBJS += out/frame.o
OBJS += out/eval.o
OBJS += out/op.o

qs: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS)

parser.h: parser.sid parser.act
parser.c: parser.sid parser.act
	$(SID) -l test parser.sid
	$(SID) $(SID_CFLAGS) parser.sid parser.act parser.c parser.h

