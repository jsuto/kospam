%define name clapf
%define version 0.5.1
%define release 1

Summary:        a high performance statistical antispam application
Name:           %{name}
Version:        %{version}
Release:        %{release}
License:        zlib/png
Group:          Networking/Mail
Source0:        master.tar.gz
URL:            http://clapf.org/
Buildroot:      /tmp/aa
BuildRequires:  openssl-devel, tcp_wrappers, mysql-devel, tre-devel
Requires:       mysql, openssl, tcp_wrappers, tre

%description
clapf is an open source and high perfomance statistical (inverse
chi-square) spam filter. It also features spam trap addresses,
support for the clamav daemon, and a nice php gui for both 
administration and quarantine for users.

%prep
%setup

%build
./configure --localstatedir=/var --with-database=mysql --enable-clamd
make clean all

%install
mkdir -p /root/rpmbuild/BUILDROOT/clapf-0.5.1-1.x86_64/etc/init.d
mkdir -p /root/rpmbuild/BUILDROOT/clapf-0.5.1-1.x86_64/usr/local/lib
make install DESTDIR=/root/rpmbuild/BUILDROOT/clapf-0.5.1-1.x86_64


%files
%defattr(-,root,root)
%dir /var/clapf
%dir /var/clapf/tmp
%dir /var/clapf/sphinx
%dir /var/clapf/queue
%dir /var/clapf/stat
%dir /var/clapf/www
%dir /var/run/clapf
/etc/init.d/rc.clapf
/etc/init.d/rc.searchd
%attr(0640,root,clapf) /usr/local/etc/clapf.conf.dist
%attr(0640,root,clapf) /usr/local/etc/sphinx.conf.dist
/usr/local/bin/spamdrop
/usr/local/bin/splitmbox
/usr/local/sbin/clapf
/usr/local/sbin/clapfconf
/usr/local/lib/libclapf.a
/usr/local/lib/libclapf.so
/usr/local/lib/libclapf.so.0
/usr/local/lib/libclapf.so.0.1.1
/usr/local/libexec/clapf/quarantine-daily-report.php
/usr/local/libexec/clapf/indexer.main.sh
/usr/local/share/clapf/db-mysql.sql
/usr/local/share/clapf/zombienets.regex


%pre
groupadd clapf
useradd -g clapf -s /bin/sh -d /var/clapf clapf
usermod -L clapf
if [ -d /var/clapf ]; then chmod 755 /var/clapf; fi


%post
chown clapf:clapf /var/run/clapf /var/clapf/tmp /var/clapf/sphinx /var/clapf/queue /var/clapf/stat
chgrp apache /var/clapf/www/tmp
chmod 775 /var/clapf/www/tmp
chmod 711 /var/clapf/queue
chmod 711 /var/clapf/tmp
echo /usr/local/lib > /etc/ld.so.conf.d/clapf.conf
ldconfig
echo this is the postinstall stuff...

%postun
userdel clapf
groupdel clapf


%changelog
* Sun Jan  4 2015 Janos Suto
  - First release of the rpm package based on build 846


