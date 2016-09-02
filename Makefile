# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
# Author: Luis.Colorado@HispaLinux.ES
# Date: sáb nov  4 22:48:46 MET 2000

prefix=$(HOME)
bindir=$(prefix)/bin

INSTALL = install
RM = rm -f
targets = gusanos

.PHONY: all clean install

all: $(targets)

gusanos_objs=gusanos.o
gusanos_libs=-lncursesw

gusanos: $(gusanos_objs)
	$(CC) $(LDFLAGS) -o gusanos $(gusanos_objs) $(gusanos_libs)

clean:
	$(RM) gusanos $(gusanos_objs)

install: $(targets)
	install -m 755 -d "$(bindir)"
	install -m 711 $(targets) "$(bindir)"

# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
