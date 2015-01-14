#!/bin/bash

DISTRO=""

HOSTNAME=`hostname -f`
SMARTHOST="127.0.0.1"
SMARTHOST_PORT="10026"
CLAPFUSER="clapf"
CLAPFGROUP="clapf"
CRON_TMP="clapf.crtab"
MYSQL_HOSTNAME="localhost"
MYSQL_DATABASE="clapf"
MYSQL_USERNAME="clapf"
MYSQL_PASSWORD="verystrongpassword"
MYSQL_ROOT_PASSWORD="mysql123"
DOCROOT="/var/clapf/www"
WWWGROUP="www-data"
SSL_CERT_DATA="/C=US/ST=Denial/L=Springfield/O=Dis/CN=www.example.com"

NGINX_CLAPF_CONFIG_DEBIAN="/etc/nginx/sites-available/clapf.conf"
NGINX_CLAPF_CONFIG_CENTOS="/etc/nginx/conf.d/clapf.conf"

MYSQL_CLAPF_CONFIG_DEBIAN="/etc/mysql/conf.d/clapf.conf"
MYSQL_CLAPF_CONFIG_CENTOS="/etc/mysql-clapf.conf"

SPHINX_CLAPF_CONFIG_DEBIAN="/etc/sphinxsearch/sphinx.conf"
SPHINX_CLAPF_CONFIG_CENTOS="/etc/sphinx/sphinx.conf"

SPHINX_VERSION=2.2.6

isFQDN() {
  # we need min. 2 dots
  if [ x"$1" = "xdogfood" ]; then
    echo 1
    return
  fi

  if [ x"$1" = "x" ]; then
    echo 0
    return
  fi

  NF=`echo $1 | awk -F. '{print NF}'`
  if [ $NF -ge 2 ]; then
    echo 1
  else
    echo 0
  fi
}


ask() {
  PROMPT=$1
  DEFAULT=$2

  echo ""
  echo -n "$PROMPT [$DEFAULT] "
  read response

  if [ -z $response ]; then
    response=$DEFAULT
  fi
}


askNonBlankNoEcho() {
  PROMPT=$1
  DEFAULT=$2

  while [ 1 ]; do
    stty -echo
    ask "$PROMPT" "$DEFAULT"
    stty echo
    echo ""
    if [ ! -z $response ]; then
      break
    fi
    echo "A non-blank answer is required"
  done
}


askNoEcho() {
  PROMPT=$1
  DEFAULT=$2

  stty -echo
  ask "$PROMPT" "$DEFAULT"
  stty echo
  echo ""
}


askNonBlank() {
  PROMPT=$1
  DEFAULT=$2

  while [ 1 ]; do
    ask "$PROMPT" "$DEFAULT"
    if [ ! -z $response ]; then
      break
    fi
    echo "A non-blank answer is required"
  done
}


askYN() {
  PROMPT=$1
  DEFAULT=$2

  if [ "x$DEFAULT" = "xyes" -o "x$DEFAULT" = "xYes" -o "x$DEFAULT" = "xy" -o "x$DEFAULT" = "xY" ]; then
    DEFAULT="Y"
  else
    DEFAULT="N"
  fi

  while [ 1 ]; do
    ask "$PROMPT" "$DEFAULT"
    response=$(perl -e "print lc(\"$response\");")
    if [ -z $response ]; then
      :
    else
      if [ $response = "yes" -o $response = "y" ]; then
        response="yes"
        break
      else
        if [ $response = "no" -o $response = "n" ]; then
          response="no"
          break
        fi
      fi
    fi
    echo "A Yes/No answer is required"
  done
}


check_user() {
   user=$1

   if [ x`whoami` != x$user ]; then echo "ERROR: the installer must be run as $user user"; exit 1; fi
}


run_detection() {

   if [ -f /etc/redhat-release ]; then DISTRO="centos"; fi
   if [ -f /etc/debian_version ]; then DISTRO="debian"; fi

   if [ x"$DISTRO" = "x" ]; then echo "Unsupported Linux distribution"; exit 1; fi

   if test "$DISTRO" = "centos"; then
      NGINX_CLAPF_CONFIG=$NGINX_CLAPF_CONFIG_CENTOS
      MYSQL_CLAPF_CONFIG=$MYSQL_CLAPF_CONFIG_CENTOS
      SPHINX_CLAPF_CONFIG=$SPHINX_CLAPF_CONFIG_CENTOS
      MYSQL_SOCKET="/var/lib/mysql/mysql.sock"
      FASTCGI_PASS='127.0.0.1:9000;'

      sed 's/-A INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT/-A INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT\n-A INPUT -m state --state NEW -m tcp -p tcp --dport 80 -j ACCEPT/' /etc/sysconfig/iptables > qqaa
      cat qqaa > /etc/sysconfig/iptables
      /etc/init.d/iptables restart
   fi

   if test "$DISTRO" = "debian"; then
      mkdir -p /etc/mysql/conf.d

      NGINX_CLAPF_CONFIG=$NGINX_CLAPF_CONFIG_DEBIAN
      MYSQL_CLAPF_CONFIG=$MYSQL_CLAPF_CONFIG_DEBIAN
      SPHINX_CLAPF_CONFIG=$SPHINX_CLAPF_CONFIG_DEBIAN
      MYSQL_SOCKET="/var/run/mysqld/mysqld.sock"
      FASTCGI_PASS='unix:/var/run/php5-fpm.sock;'

   fi

}


get_mysql_root_pwd() {

   askNoEcho "Please enter mysql root password" ""
   MYSQL_ROOT_PASSWORD=$response

   s=`echo "use information_schema; select TABLE_NAME from TABLES where TABLE_SCHEMA='$MYSQL_DATABASE'" | mysql -h $MYSQL_HOSTNAME -u root --password=$MYSQL_ROOT_PASSWORD`
   if [ $? -eq 0 ];
   then
      echo "mysql connection successful"; echo;
      if [ `echo $s | grep -c history` -eq 1 ]; then echo "ERROR: Detected history table in $MYSQL_DATABASE. Aborting"; exit 0; fi
   else
      echo "ERROR: failed to connect to mysql";
      get_mysql_root_pwd
   fi

}


collect_data() {
   askNonBlank "Please enter FQDN of this host" "$HOSTNAME"
   HOSTNAME=$response

   ask "Please enter the FQDN / IP-address of the smarthost" "$SMARTHOST"
   SMARTHOST=$response

   ask "Please enter the tcp port of the smarthost" "$SMARTHOST_PORT"
   SMARTHOST_PORT=$response

   askNonBlank "Please enter the group of the webserver" "$WWWGROUP"
   WWWGROUP=$response

   askNonBlank "Please enter SSL certificate data for the clapf daemon" "$SSL_CERT_DATA"
   SSL_CERT_DATA=$response

   askNonBlankNoEcho "Please enter mysql password for $MYSQL_USERNAME" ""
   MYSQL_PASSWORD=$response

   get_mysql_root_pwd

   show_install_parameters

   askYN "Accept and continue? [Y/n]: " "Y"
   YN=$response

   if test "$YN" = "no"; then collect_data; fi

   hostname $HOSTNAME

}



show_install_parameters() {
   echo
   echo "Configuration details:"
   echo "----------------------"
   echo
   echo "Hostname: $HOSTNAME"
   echo "Smarthost: $SMARTHOST:$SMARTHOST_PORT"
   echo "Documentroot: $DOCROOT"
   echo "Webserver user: $WWWGROUP"
   echo "SSL cert data: $SSL_CERT_DATA"
}


show_licence() {
   echo
   echo "This is the clapf install script"
   echo
   echo "The install script is designed to work on a fresh minimal install"
   echo
   echo "LICENCE"
   echo "-------"
   echo
cat <<AAAA
Copyright (C) 2004-2015, Janos SUTO <sj@acts.hu>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
AAAA

   echo
   echo
   echo "Enter Y to accept the licence and continue."
   echo
   askYN "Accept licence? [y/N]: " "N"
   YN=$response

   if test "$YN" != "yes"; then echo "Aborted."; exit 1; fi
}


fix_mysql_config() {

cat <<MYSQL > $MYSQL_CLAPF_CONFIG

[mysqld]

innodb_buffer_pool_size = 256M
innodb_additional_mem_pool_size = 8M

innodb_log_file_size = 5M
innodb_lock_wait_timeout = 50

innodb_support_xa=0
innodb_flush_log_at_trx_commit=2
innodb_log_buffer_size=16M
innodb_log_file_size=16M

innodb_read_io_threads=4
innodb_write_io_threads=4
innodb_log_files_in_group=2

innodb_file_per_table 

MYSQL

}


install_debian_packages() {
   apt-get install openssl libssl-dev gcc g++ make sysstat libtre-dev php5-cli php5-cgi php5-mysql php5-fpm php5-ldap php5-gd php5-xcache php5-curl libwrap0-dev libmysqlclient-dev libzip-dev nginx mysql-server clamav-daemon

   wget http://sphinxsearch.com/files/sphinx-$SPHINX_VERSION-release.tar.gz
   tar zxvf sphinx-$SPHINX_VERSION-release.tar.gz
   cd sphinx-$SPHINX_VERSION-release
   ./configure --prefix=/usr --sysconfdir=/etc/sphinxsearch
   make
   make install
   cd ..
}



write_nginx_vhost() {

cat <<NGINX > $NGINX_CLAPF_CONFIG


    server {
        server_name  $HOSTNAME;

        root $DOCROOT;

        access_log  /var/log/nginx/$HOSTNAME-access.log;
        error_log   /var/log/nginx/$HOSTNAME-error.log;

        location / {
            index  index.php index.html;
        }

        location ~ \.php\$ {
            fastcgi_pass   $FASTCGI_PASS
            fastcgi_index  index.php;
            fastcgi_param  SCRIPT_FILENAME  \$document_root\$fastcgi_script_name;
            include        fastcgi_params;
        }

        rewrite /search.php /index.php?route=search/search&type=simple;
        rewrite /advanced.php /index.php?route=search/search&type=advanced;
        rewrite /expert.php /index.php?route=search/search&type=expert;
        rewrite /search-helper.php /index.php?route=search/helper;
        rewrite /history-helper.php /index.php?route=history/helper;
        rewrite /audit-helper.php /index.php?route=audit/helper;
        rewrite /message.php /index.php?route=message/view;
        rewrite /bulkrestore.php /index.php?route=message/bulkrestore;
        rewrite /bulkpdf.php /index.php?route=message/bulkpdf;
        rewrite /folders.php /index.php?route=folder/list&;
        rewrite /settings.php /index.php?route=user/settings;
        rewrite /login.php /index.php?route=login/login;
        rewrite /logout.php /index.php?route=login/logout;
        rewrite /google.php /index.php?route=login/google;
        rewrite /domain.php /index.php?route=domain/domain;
        rewrite /ldap.php /index.php?route=ldap/list;
        rewrite /customer.php /index.php?route=customer/list;
        rewrite /retention.php /index.php?route=policy/retention;
        rewrite /archiving.php /index.php?route=policy/archiving;
        rewrite /view/javascript/piler.js /js.php;
    }


NGINX


}


create_clapf_user() {
   groupadd $CLAPFGROUP
   useradd -g $CLAPFGROUP -s /bin/sh -d /var/clapf $CLAPFUSER
   usermod -L $CLAPFUSER
   if [ -d /var/clapf ]; then chmod 755 /var/clapf; fi
}


download_and_install_clapf() {

   echo "Downloading clapf . . ."
   wget -q --no-check-cert https://bitbucket.org/jsuto/clapf/get/master.tar.gz
   tar zxf master.tar.gz
   cd jsuto-clapf-*
   ./configure --localstatedir=/var --with-database=mysql --enable-clamd
   make clean all
   make install
   ldconfig

   cp init.d/rc.clapf /etc/init.d/rc.clapf
   cp init.d/rc.searchd /etc/init.d/rc.searchd
   chmod +x /etc/init.d/rc.clapf /etc/init.d/rc.searchd

cat <<CRONTAB > $CRON_TMP
### CLAPFSTART
*/15 * * * * /usr/local/libexec/clapf/indexer.main.sh
2    2 * * * find /var/clapf/queue -type f -ctime +30 -exec rm -f '{}' \;
### CLAPFEND

CRONTAB

}


configure_clapf() {


cat <<CLAPFCONF > /usr/local/etc/clapf.conf
blackhole_email_list=
group_type=0
locale=
max_message_size_to_filter=100000
max_number_of_recipients_in_ham=300
max_number_of_tokens_to_filter=6000
mydomains=
mydomains_from_outside_is_spam=0
mynetwork=
mysqlsocket=$MYSQL_SOCKET
mysqluser=clapf
mysqlpwd=$MYSQL_PASSWORD
mysqldb=clapf
our_signo=X-Anti-Backscatter: xxxxxxxxxxxx
penalize_embed_images=1
penalize_images=1
penalize_octet_stream=1
possible_spam_limit=0.8000
possible_spam_subject_prefix=[spam???] 
server_id=
smtp_addr=$SMARTHOST
smtp_port=$SMARTHOST_PORT
skipped_received_ips=
store_emails=1
store_only_spam=0
surbl_domain=multi.surbl.org
training_mode=0
update_tokens=1
verbosity=1
CLAPFCONF

chmod 640 /usr/local/etc/clapf.conf
chgrp $CLAPFGROUP /usr/local/etc/clapf.conf
}


clapf_postinstall() {

echo -n "Creating mysql database... ";

echo "create database $MYSQL_DATABASE character set 'utf8'" | mysql -h $MYSQL_HOSTNAME -u root --password=$MYSQL_ROOT_PASSWORD
echo "grant all privileges on $MYSQL_DATABASE.* to $MYSQL_USERNAME@localhost identified by '$MYSQL_PASSWORD'" | mysql -h $MYSQL_HOSTNAME -u root --password=$MYSQL_ROOT_PASSWORD
echo "flush privileges" | mysql -h $MYSQL_HOSTNAME -u root --password=$MYSQL_ROOT_PASSWORD
echo "Done."


echo -n "Creating tables for clapf... ";
mysql -h $MYSQL_HOSTNAME -u $MYSQL_USERNAME --password=$MYSQL_PASSWORD $MYSQL_DATABASE < util/db-mysql.sql
echo "Done."

echo -n "Overwriting sphinx configuration... ";
sed -e "s%MYSQL_HOSTNAME%$MYSQL_HOSTNAME%" -e "s%MYSQL_DATABASE%$MYSQL_DATABASE%" -e "s%MYSQL_USERNAME%$MYSQL_USERNAME%" -e "s%MYSQL_PASSWORD%$MYSQL_PASSWORD%" etc/sphinx.conf.in > etc/sphinx.conf
cp etc/sphinx.conf $SPHINX_CLAPF_CONFIG
echo "Done."

echo -n "Initializing sphinx indices... ";
su $CLAPFUSER -c "indexer --all"
echo "Done."


echo -n "installing cron entries for $CLAPFUSER... "
crontab -u $CLAPFUSER $CRON_TMP
echo "Done."

echo -n "Making an ssl certificate ... "
openssl req -new -newkey rsa:4096 -days 3650 -nodes -x509 -subj "$SSL_CERT_DATA" -keyout /usr/local/etc/clapf.pem -out 1.cert
cat 1.cert >> /usr/local/etc/clapf.pem
chmod 600 /usr/local/etc/clapf.pem
rm 1.cert


chmod 755 /var/clapf/stat

echo -n "Copying www files to $DOCROOT... "
mkdir -p $DOCROOT || exit 1

cp -R webui/* $DOCROOT
cp -R webui/.htaccess $DOCROOT
chmod 770 $DOCROOT/tmp $DOCROOT/images
chown $CLAPFUSER $DOCROOT/tmp
chgrp $WWWGROUP $DOCROOT/tmp
chgrp $WWWGROUP $DOCROOT/images


cat <<CONFIGSITE > $DOCROOT/config-site.php
<?php

\$config['SITE_NAME'] = '$HOSTNAME';
\$config['SITE_URL'] = 'http://' . \$config['SITE_NAME'] . '/';
\$config['DIR_BASE'] = '$DOCROOT/';

\$config['TIMEZONE'] = 'Europe/Budapest';

\$config['SMTP_DOMAIN'] = '$HOSTNAME';
\$config['SMTP_FROMADDR'] = 'no-reply@$HOSTNAME';
\$config['ADMIN_EMAIL'] = 'admin@$HOSTNAME';
\$config['HAM_TRAIN_ADDRESS'] = 'ham@$HOSTNAME';
\$config['SPAM_TRAIN_ADDRESS'] = 'spam@$HOSTNAME';

\$config['DB_DRIVER'] = 'mysql';
\$config['DB_PREFIX'] = '';
\$config['DB_HOSTNAME'] = '$MYSQL_HOSTNAME';
\$config['DB_USERNAME'] = '$MYSQL_USERNAME';
\$config['DB_PASSWORD'] = '$MYSQL_PASSWORD';
\$config['DB_DATABASE'] = '$MYSQL_DATABASE';

\$config['SMARTHOST'] = '$SMARTHOST';
\$config['SMARTHOST_PORT'] = $SMARTHOST_PORT;

?>
CONFIGSITE

}


debian_fix() {
   (cd /etc/nginx/sites-enabled; ln -sf $NGINX_CLAPF_CONFIG_DEBIAN)
}

centos_fix() {
   chkconfig exim off
   chkconfig php-fpm on
   chkconfig nginx on
   chkconfig searchd off
   chkconfig memcached on

   WWWGROUP="apache"

   ###(echo; echo \!include\ $MYSQL_CLAPF_CONFIG_CENTOS) >> /etc/my.cnf
   echo /usr/local/lib > /etc/ld.so.conf.d/clapf.conf

   if [ -f /etc/init.d/mysqld ]; then /etc/init.d/mysqld start; chkconfig mysqld on; fi
   if [ -f /etc/init.d/mysql ]; then /etc/init.d/mysql start; chkconfig mysql on; fi

   setsebool -P httpd_can_network_connect=1
}


write_etc_rc_local() {

cat <<RCLOCAL > /etc/rc.local
#!/bin/sh -e
#
# rc.local
#

/etc/init.d/rc.clapf start
/etc/init.d/rc.searchd start

exit 0
RCLOCAL

}





run_detection

###echo "distro: $DISTRO"

check_user root
show_licence


fix_mysql_config

if test "$DISTRO" = "debian"; then
   install_debian_packages
fi

if test "$DISTRO" = "centos"; then
   install_centos_packages
   centos_fix
fi

create_clapf_user


collect_data


write_nginx_vhost



download_and_install_clapf

configure_clapf

clapf_postinstall


if test "$DISTRO" = "debian"; then
   debian_fix
fi

#if test "$DISTRO" = "centos"; then
#   centos_fix
#fi





write_etc_rc_local

