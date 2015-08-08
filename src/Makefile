SID ?= sid
CC  ?= gcc

CFLAGS += -ansi -pedantic
CFLAGS += -Wall -W
#CFLAGS += -Werror
CFLAGS += -Og -g -ggdb

SID_CFLAGS += -s no-numeric-terminals -s no-terminals

OBJS += dup.o
OBJS += lex.o
OBJS += parser.o
OBJS += main.o
OBJS += var.o
OBJS += frame.o
OBJS += code.o
OBJS += data.o
OBJS += readfd.o
OBJS += eval.o
OBJS += args.o
OBJS += pair.o
OBJS += proc.o
OBJS += status.o
OBJS += builtin.o
OBJS += signal.o
OBJS += task.o

qs: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS) qs

parser.h: parser.sid parser.act
parser.c: parser.sid parser.act
	$(SID) -l test parser.sid
	$(SID) $(SID_CFLAGS) parser.sid parser.act parser.c parser.h

signal.o: signum.h
signum.h:
	echo 'static const struct signum signum[] = {' > $@
	kill -l                                \
	| grep -v '^[0-9]'                     \
	| grep -v '^RT'                        \
	| sort                                 \
	| while read s; do                     \
	    l=`echo $$s | tr '[A-Z]' '[a-z]'`; \
	    printf "\t{ %-12s %-9s },\n"       \
	        '"'sig$$l'"', SIG$$s;          \
	done                                   \
	| sed '$$s/.$$//' >> $@
	echo '};' >> $@

