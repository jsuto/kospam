%define name	clapf
%define version	0.3.28
%define release	1

Summary:	a modular network filter for postfix
Name:		%{name}
Version:	%{version}
Release:	%{release}
License:	zlib/png
Group:		Networking/Mail
Source0:	%{name}-%{version}.tar.gz
Source1:	%{name}.init
URL:		http://clapf.acts.hu/
Buildroot:	%{_tmppath}/%{name}-%{version}-root
BuildRequires:	mysql-devel, tinycdb, clamd
Requires:	mysql, tinycdb, clamd, postfix, perl-CDB_File, openldap-devel

%description
clapf is a modular network filter for postfix. It includes a statisti-
cal (inverse chi-square) antispam module, the  blackhole  feature  and
antivirus support (clamav, AVG Linux, Dr.Web, avast! and Kasperky) pre
venting virus infection as well as a cgi and a command line  interface
to easily train the token database.


%prep


%setup -q -n %{name}-%{version}
CPPFLAGS=-I/usr/include/mysql LDFLAGS=-L/usr/lib ./configure --prefix=/usr --enable-clamd --enable-cgi --enable-surbl --with-userdb=mysql


%build
make clean all


%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot} 

install -d %{buildroot}/etc/init.d
install -m 755 %{SOURCE1} %{buildroot}/etc/init.d/%{name}

cat example.conf | sed 's/^workdir=.*$/workdir=\/var\/spool\/clapf/g' | sed 's/^quarantine_dir=.*$/quarantine_dir=\/var\/spool\/clapf\/quarantine/g' | sed 's/^spam_quarantine_dir=.*$/spam_quarantine_dir=\/var\/spool\/clapf\/spamquarantine/g' | sed 's/^tokensfile=.*/tokensfile=\/var\/spool\/clapf\/tokens.cdb/g' | sed 's/^raw_text_datafile=.*/raw_text_datafile=\/var\/spool\/clapf\/tokens.raw/g' > example2.conf
install -m 644 example2.conf %{buildroot}/etc/%{name}.conf

install -d %{buildroot}/var/spool/clapf/quarantine
install -d %{buildroot}/var/spool/clapf/spamquarantine
install -d %{buildroot}/var/spool/clapf/tmp

install -d %{buildroot}/usr/bin
cd $RPM_BUILD_DIR/%{name}-%{version}
for i in clapf parsembox spamcgi spamtest splitmbox train traincgi
do
	install -m 755 $i %{buildroot}/usr/bin
done

cd doc/man
install -d %{buildroot}%{_mandir}/man8
for i in `ls *.8`
do
	install -m 644 $i %{buildroot}%{_mandir}/man8
done

install -d %{buildroot}%{_mandir}/man1
for i in `ls *.1`
do
	install -m 644 $i %{buildroot}%{_mandir}/man1
done

#doc/html/*

cd ../..
for i in blackhole perl stat TRAINING util 
do
	install -d %{buildroot}%{_docdir}/%{name}-%{version}/contrib/$i
	install -m 644 $i/* %{buildroot}%{_docdir}/%{name}-%{version}/contrib/$i
done
cp doc/LICENSE .


%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot} 


%pre
/usr/sbin/groupadd -r clapf 2>/dev/null || :
/usr/sbin/useradd -r -d /var/spool/clapf -s /sbin/sh -c "Clapf Antispam" -g clapf clapf 2>/dev/null || :
/usr/bin/passwd -l clapf


%post
/sbin/chkconfig --add clapf


%preun
if [ $1 -eq 0 ]; then
        /sbin/service clapf stop &>/dev/null || :
        /sbin/chkconfig --del clapf
fi


%files
%defattr(-,root,root)
%doc README LICENSE Changelog blackhole perl stat TRAINING util
%config %attr(0644,root,root) /etc/%{name}.conf
%attr(0755,root,root) /etc/init.d/clapf
%attr(0755,root,root) %{_bindir}
%attr(0644,root,root) %{_mandir}
%dir %attr(0711,clapf,clapf) /var/spool/clapf
%dir %attr(0751,clapf,clapf) /var/spool/clapf/quarantine
%dir %attr(0751,clapf,clapf) /var/spool/clapf/tmp
%dir %attr(0770,clapf,apache) /var/spool/clapf/spamquarantine


%changelog
* Thu May 25 2006 Tim Philips <timp@thetimp.co.nz> - 0.3.26-1
- Rebuilt with updated version.
- Require openldap-devel
- Enable SURBL

* Thu May 18 2006 Tim Philips <timp@thetimp.co.nz> - 0.3.25-1
- First build
