CFLAGS=	-ansi -pedantic -Wall -W -Werror

OBJS += lex.o
OBJS += parse.o
OBJS += bet.o
OBJS += exec.o
OBJS += builtin.o
OBJS += main.o

kcsh: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS)

