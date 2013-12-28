CFLAGS=	-ansi -pedantic -Wall -W -Werror

OBJS += lex.o
OBJS += parse.o
OBJS += ast_new.o
OBJS += ast_free.o
OBJS += ast_dump.o
OBJS += exec.o
OBJS += builtin.o
OBJS += main.o

kcsh: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS)

