# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
# Author: Luis.Colorado@HispaLinux.ES
# Date: sab nov  4 22:48:46 MET 2000

prefix=/usr/local
execprefix=$(prefix)
bindir=$(prefix)/bin
datarootdir=$(prefix)/share
mandir=$(datarootdir)/man
man1dir=$(mandir)/man1

INSTALL            ?= install
RM                 ?= rm -f
targets            ?= worms
LIBS               ?=-lncursesw
GZIP               ?= gzip -v

OS                 != uname -s

OWN-FreeBSD        ?= root
GRP-FreeBSD        ?= wheel
OWN-GNU/Linux      ?= bin
GRP-GNU/Linux      ?= bin

own                ?= $(OWN-$(OS))
grp                ?= $(GRP-$(OS))

fmod               ?= 0444
xmod               ?= 0555
dmod               ?= 0555

LIBS-FreeBSD        = -lncursesw
LIBS-Linux          = -lncursesw
LIBS-Cygwin         = -lncursesw

LIBS                = $(LIBS-$(OS))

INSTALL             = install
RM                 ?= rm -f
targets             = worms
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
	$(RM) $(toinstall)

worms: $(worms_objs)
	$(CC) $(LDFLAGS) -o $@ $($@_objs) $(LIBS)

$(bindir)/worms: worms
	$(INSTALL) -o $(own) -g $(grp) -m $(xmod) worms $@

$(man1dir)/worms.1.gz: worms.1.gz
	$(INSTALL) -o $(own) -g $(grp) -m $(fmod) worms.1.gz $@

worms.1.gz: worms.1
	$(GZIP) < worms.1 > $@

# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
