.include "../share/mk/top.mk"

SRC += src/dup.c
SRC += src/lex.c
SRC += src/parser.c
SRC += src/main.c
SRC += src/var.c
SRC += src/frame.c
SRC += src/code.c
SRC += src/data.c
SRC += src/readbuf.c
SRC += src/eval.c
SRC += src/args.c
SRC += src/pair.c
SRC += src/proc.c
SRC += src/status.c
SRC += src/builtin.c
SRC += src/signal.c
SRC += src/task.c

PARSER += src/parser.sid

PROG += qs

INCDIR_src/signal.c += ${BUILD}/src

.for src in ${SRC:Msrc/*.c}
${BUILD}/bin/qs: ${BUILD}/${src:R}.o
.endfor

# not committed, because it's specific to your system
${BUILD}/src/signal.o: ${BUILD}/src/siglist.h
${BUILD}/src/siglist.h:
	echo 'static const struct siglist siglist[] = {' > $@
	${KILL} -l                             \
	| grep -v '^[0-9]'                     \
	| grep -v '^RT'                        \
	| tr ' ' '\n'                          \
	| sort                                 \
	| while read s; do                     \
	    l=`echo $$s | tr '[A-Z]' '[a-z]'`; \
	    printf "\t{ %-12s %-9s },\n"       \
	        '"'sig$$l'"', SIG$$s;          \
	done                                   \
	| sed '$$s/.$$//' >> $@
	echo '};' >> $@

