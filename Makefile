.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep prog
dep::
gen::
test:: all
install:: all
clean::

# things to override
CC     ?= gcc
BUILD  ?= build
PREFIX ?= /usr/local
KILL   ?= /bin/kill

# layout
SUBDIR += aux
SUBDIR += src

.include <subdir.mk>
.include <sid.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <so.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

