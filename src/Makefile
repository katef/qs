CFLAGS=	-ansi -pedantic -Wall -W -Werror

OBJS += lex.o
OBJS += parse.o
OBJS += ast.o
OBJS += ast_dump.o
OBJS += main.o

kcsh: $(OBJS)
	$(CC) -o $@ $>

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS)

