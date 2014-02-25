SID ?= sid
CC  ?= gcc

CFLAGS += -ansi -pedantic
#CFLAGS += -Wall -W -Werror
CFLAGS += -O2 -g -ggdb

SID_CFLAGS += -s no-numeric-terminals -s no-terminals

OBJS += lex.o
OBJS += parser.o
OBJS += ast.o
#OBJS += exec.o
OBJS += builtin.o
OBJS += main.o
OBJS += var.o
OBJS += scope.o

qs: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS)

parser.h: parser.sid parser.act
parser.c: parser.sid parser.act
	$(SID) -l test parser.sid
	$(SID) $(SID_CFLAGS) parser.sid parser.act parser.c parser.h

