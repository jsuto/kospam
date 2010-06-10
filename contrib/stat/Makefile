SHELL = /bin/sh

include Makefile.helper

CC = gcc
CFLAGS = -static -O2 -Wall -g 
DEFS =  -DHAVE_MYSQL -D_GNU_SOURCE -DHAVE_ANTIVIRUS -DHAVE_CLAMD -DHAVE_ANTISPAM -DHAVE_RBL -DUSERS_IN_MYSQL -DHAVE_USERS -DHAVE_STORE -DSTORE_FS -DHAVE_WHITELIST -DHAVE_LMTP -DHAVE_MEMCACHED -DHAVE_POLICY -DHAVE_TRE -DNEED_MYSQL
INCDIR = -I. -I../.. -I../../src  -I/usr/local/mysql/include
LIBDIR = -L.   -L../../src
LIBS =   -lm -ldl -ltre    -L/usr/local/mysql/lib -lmysqlclient_r -lz -lpthread -lcrypt -lnsl -lm -lpthread
LDAP_LIBS = 

all:
	cat $(srcdir)/clapf-stat.run.sh.in | sed -e "s%YOUR_LIBEXECDIR%$(libexecdir)%g" -e "s%YOUR_LOCALSTATEDIR%$(localstatedir)%g" > $(srcdir)/clapf-stat.run.sh

install:

	$(INSTALL) -d -m 0755 $(DESTDIR)/$(libexecdir)/clapf

	$(INSTALL) -m 0755 $(srcdir)/clapf-rrd-create.sh $(DESTDIR)$(libexecdir)/clapf
	$(INSTALL) -m 0755 $(srcdir)/clapf-rrd-graph-oneline.sh $(DESTDIR)$(libexecdir)/clapf
	$(INSTALL) -m 0755 $(srcdir)/clapf-rrd-graph.sh $(DESTDIR)$(libexecdir)/clapf
	$(INSTALL) -m 0755 $(srcdir)/clapf-rrd-update.sh $(DESTDIR)$(libexecdir)/clapf
	$(INSTALL) -m 0755 $(srcdir)/process_syslog.pl $(DESTDIR)$(libexecdir)/clapf
	$(INSTALL) -m 0755 $(srcdir)/clapf-stat.run.sh $(DESTDIR)$(libexecdir)/clapf
clean:
	rm -f clapf-stat.run.sh

distclean: clean
	rm -f Makefile.helper
