.include "../share/mk/top.mk"

PROG += bg
PROG += echo
PROG += false
PROG += glob
PROG += ppid
PROG += true

.for prog in ${PROG}
SRC += aux/${prog}.c
.endfor

.for prog in ${PROG}
${BUILD}/bin/${prog}: ${BUILD}/aux/${prog}.o
.endfor

