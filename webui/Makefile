SHELL = /bin/sh

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
srcdir = .
sysconfdir = ${prefix}/etc
mandir = ${datarootdir}/man
datarootdir = ${prefix}/share
localstatedir = /var

CC = gcc
CFLAGS =  -O2 -Wall -g 
DEFS =  -D_GNU_SOURCE -DHAVE_TRE -DNEED_MYSQL
INCDIR = -I. -I.. -I../.. -I../../src -I../src  @mysql_includes@
LIBDIR = -L.   -L../src -L../../src
LIBS = -lz -lm -ldl -lcrypto -lssl -ltre -lzip -lwrap -lnsl   @mysql_libs@
OBJS = 

INSTALL = /bin/ginstall -c

all:

install:
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/Zend
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/bootstrap
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/controller
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/google-api
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/images
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/language
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/model
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/securimage
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/system
	$(INSTALL) -d -m 0755 $(DESTDIR)$(localstatedir)/www/piler/view
	$(INSTALL) $(srcdir)/.htaccess $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) $(srcdir)/config.php $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) $(srcdir)/index.php $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) $(srcdir)/js.php $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) $(srcdir)/qr.php $(DESTDIR)$(localstatedir)/www/piler
	$(INSTALL) $(srcdir)/sso.php $(DESTDIR)$(localstatedir)/www/piler
	cp -R $(srcdir)/Zend/* $(DESTDIR)$(localstatedir)/www/piler/Zend
	cp -R $(srcdir)/bootstrap/* $(DESTDIR)$(localstatedir)/www/piler/bootstrap
	cp -R $(srcdir)/controller/* $(DESTDIR)$(localstatedir)/www/piler/controller
	cp -R $(srcdir)/google-api/* $(DESTDIR)$(localstatedir)/www/piler/google-api
	cp -R $(srcdir)/language/* $(DESTDIR)$(localstatedir)/www/piler/language
	cp -R $(srcdir)/model/* $(DESTDIR)$(localstatedir)/www/piler/model
	cp -R $(srcdir)/securimage/* $(DESTDIR)$(localstatedir)/www/piler/securimage
	cp -R $(srcdir)/system/* $(DESTDIR)$(localstatedir)/www/piler/system
	cp -R $(srcdir)/view/* $(DESTDIR)$(localstatedir)/www/piler/view


clean:

distclean: clean
	rm -f Makefile
