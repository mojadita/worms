# $Id: Makefile,v 1.1 2000/11/04 21:50:40 luis Exp $
# Author: Luis.Colorado@HispaLinux.ES
# Date: sáb nov  4 22:48:46 MET 2000

gusanos_objs=gusanos.o
gusanos_libs=-lncurses

gusanos: $(gusanos_objs)
	$(CC) $(LDFLAGS) -o gusanos $(gusanos_objs) $(gusanos_libs)

# $Id: Makefile,v 1.1 2000/11/04 21:50:40 luis Exp $
