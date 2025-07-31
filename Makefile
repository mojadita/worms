# Makefile -- script to build worms.
# Author: Luis Colorado <luiscoloradourcola@gmail.com>
# Date: Sat nov  4 22:48:46 MET 2000

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

OS                 != uname -o

OWN-FreeBSD        ?= root
GRP-FreeBSD        ?= wheel
OWN-GNU/Linux      ?= bin
GRP-GNU/Linux      ?= bin

own                ?= $(OWN-$(OS))
grp                ?= $(GRP-$(OS))

fmod               ?= 0444
xmod               ?= 0555
dmod               ?= 0555

.PHONY: all clean install
.SUFFIXES: .1.gz .1

all: $(targets)

worms_objs          =worms.o
toclean            += $(worms_objs)

worms: $(worms_objs)
	$(CC) $(LDFLAGS) -o $@ $($@_objs) $(LIBS)
toclean            += worms

clean:
	$(RM) $(toclean)

toinstall = $(bindir)/worms $(man1dir)/worms.1.gz

install: $(toinstall)

uninstall deinstall:
	$(RM) $(toinstall)

$(bindir)/worms: worms
	$(INSTALL) -o $(own) -g $(grp) -m $(xmod) worms $(bindir)

$(man1dir)/worms.1.gz: worms.1.gz
	$(INSTALL) -o $(own) -g $(grp) -m $(fmod) worms.1.gz $(man1dir)

$(bindir) $(man1dir):
	$(INSTALL) -o $(own) -g $(grp) -m $(dmod) -d $@

.1.1.gz:
	$(GZIP) < $< > $@

# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
