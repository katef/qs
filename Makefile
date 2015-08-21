.MAKEFLAGS: -r -m share

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

.include <mk/subdir.mk>
.include <mk/sid.mk>
.include <mk/obj.mk>
.include <mk/dep.mk>
.include <mk/ar.mk>
.include <mk/so.mk>
.include <mk/prog.mk>
.include <mk/mkdir.mk>
.include <mk/install.mk>
.include <mk/clean.mk>

