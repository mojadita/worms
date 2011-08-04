# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
# Author: Luis.Colorado@HispaLinux.ES
# Date: sáb nov  4 22:48:46 MET 2000

gusanos_objs=gusanos.o
gusanos_libs=-lncurses

gusanos: $(gusanos_objs)
	$(CC) $(LDFLAGS) -o gusanos $(gusanos_objs) $(gusanos_libs)

clean:
	$(RM) gusanos $(gusanos_objs)

# $Id: Makefile,v 1.2 2011/08/04 12:47:36 luis Exp $
