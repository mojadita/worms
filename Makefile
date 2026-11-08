# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
# Author: Luis.Colorado@HispaLinux.ES
# Date: sab nov  4 22:48:46 MET 2000
# Copyright: (C) 1980-2026 Luis Colorado.  All rights reserved.
# License: BSD v2

prefix=/usr/local
execprefix=$(prefix)
bindir=$(prefix)/bin
datarootdir=$(prefix)/share
mandir=$(datarootdir)/man
man1dir=$(mandir)/man1

INSTALL            ?= install
RM                 ?= rm -f
GZIP               ?= gzip -v

OS                 != uname -o

OWN-FreeBSD        ?= -o root
GRP-FreeBSD        ?= -g wheel
OWN-GNU/Linux      ?= -o bin
GRP-GNU/Linux      ?= -g bin

LIBS-Freebsd       ?= -lncursesw
LIBS-GNU/Linux     ?= -lncursesw
LIBS-Cygwin        ?= -lncurses
LIBS               ?= $(LIBS-$(OS))

own                ?= $(OWN-$(OS))
grp                ?= $(GRP-$(OS))

fmod               ?= -m 0444
xmod               ?= -m 0555
dmod               ?= -m 0555

LIBS-FreeBSD        = -lncursesw
LIBS-Linux          = -lncursesw
LIBS-Cygwin         = -lncurses

LIBS                = $(LIBS-$(OS))

INSTALL             = install
RM                 ?= rm -f

targets             = worms worms.1.gz
toclean            += $(targets)

to_install          = $(bindir)/worms \
                      $(man1dir)/worms.1.gz

worms_objs          = worms.o
worms_libs=-lncursesw

toclean            += $(worms_objs)

all: $(targets)

clean:
	$(RM) $(toclean)

install: $(to_install)

uninstall deinstall:
	-$(RM) $(to_install)

worms: $(worms_objs)
	$(CC) $(LDFLAGS) -o $@ $($@_objs) $(LIBS)

$(bindir)/worms: worms
	-$(INSTALL) $(own) $(grp) $(xmod) worms $@

$(man1dir)/worms.1.gz: worms.1.gz
	-$(INSTALL) $(own) $(grp) $(fmod) worms.1.gz $@

worms.1.gz: worms.1
	$(GZIP) < worms.1 > $@
