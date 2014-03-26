<?php


$config = array();


/*
 * you can override any of these values by putting the
 * variable to be overridden in config-site.php
 */

$config['SITE_NAME'] = 'clapf.yourdomain.com';
$config['SITE_URL'] = 'http://clapf.yourdomain.com/';

$config['BRANDING_TEXT'] = '';
$config['BRANDING_URL'] = '';

$config['TIMEZONE'] = 'Europe/Budapest';
$config['DATE_FORMAT'] = '(Y.m.d.)';

$config['ENABLE_SYSLOG'] = 1;

$config['TRAIN_DELIVERED_SPAM'] = 0;

$config['LANG'] = 'en';
$config['THEME'] = 'default';

$config['WEBUI_DIRECTORY'] = '';
$config['QUEUE_DIRECTORY'] = '/var/lib/clapf/queue';
$config['STAT_DIRECTORY'] = '/var/lib/clapf/stat';

$config['PAGE_LEN'] = 20;
$config['TEXT_SHORT_LENGTH'] = 50;

$config['LOCALHOST'] = '127.0.0.1';
$config['POSTFIX_LISTEN_ADDRESS'] = '0.0.0.0';
$config['POSTFIX_LISTEN_PORT'] = 25;
$config['POSTFIX_PORT_AFTER_CONTENT_FILTER'] = 10026;
$config['CLAPF_PORT'] = 10025;
$config['SMTP_DOMAIN'] = 'yourdomain.com';
$config['SMTP_FROMADDR'] = 'no-reply@yourdomain.com';
$config['HAM_TRAIN_ADDRESS'] = 'ham@yourdomain.com';
$config['SPAM_TRAIN_ADDRESS'] = 'spam@yourdomain.com';

$config['DIR_BASE'] = '/var/www/clapf.yourdomain.com/';

$config['DB_DRIVER'] = 'mysql';
$config['DB_PREFIX'] = '';
$config['DB_HOSTNAME'] = 'localhost';
$config['DB_USERNAME'] = 'clapf';
$config['DB_PASSWORD'] = 'changeme';
$config['DB_DATABASE'] = 'clapf';

$config['HISTORY_DRIVER'] = 'mysql';
$config['HISTORY_DATABASE'] = 'clapf';
$config['HISTORY_HOSTNAME'] = 'localhost';
$config['HISTORY_USERNAME'] = 'clapf';
$config['HISTORY_PASSWORD'] = 'changeme';
$config['HISTORY_LATEST_TIME_RANGE'] = 86400;


$config['CLAPF_HEADER_FIELD'] = 'X-Clapf-spamicity: ';

$config['ENABLE_LDAP_IMPORT_FEATURE'] = 1;
$config['LDAP_IMPORT_CONFIG_FILE'] = '/usr/local/etc/clapf-ldap-import.cfg';

$config['DN_MAX_LEN'] = 255;
$config['USE_EMAIL_AS_USERNAME'] = 1;
$config['LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK'] = 100;

$config['LDAP_HOST'] = '192.168.1.100';
$config['LDAP_BASE_DN'] = 'DC=example,DC=com';
$config['LDAP_BIND_DN'] = 'CN=Clapf User,OU=Users,DC=example,DC=com';
$config['LDAP_BIND_PW'] = '*********';


$config['QUEUE_DIR_SPLITTING'] = 0;

$config['ENABLE_BLACKLIST'] = 1;
$config['ENABLE_STATISTICS'] = 1;
$config['ENABLE_HISTORY'] = 1;
$config['ENABLE_REMOTE_IMAGES'] = '0';

$config['HELPURL'] = '';

$config['QUARANTINE_DRIVER'] = 'mysql';
$config['REMOVE_FROM_QUARANTINE_WILL_UNLINK_FROM_FILESYSTEM'] = 0;

$config['LOG_DATE_FORMAT'] = 'd-M-Y H:i:s';

$config['MIN_PASSWORD_LENGTH'] = 6;

$config['CGI_INPUT_FIELD_WIDTH'] = 50;

$config['PASSWORD_CHANGE_ENABLED'] = 1;

$config['SPAM_ONLY_QUARANTINE'] = 1;

$config['MEMCACHED_ENABLED'] = 1;



require_once 'config-site.php';

if(isset($_SESSION['theme']) && preg_match("/^([a-zA-Z0-9\-\_]+)$/", $_SESSION['theme'])) { $config['THEME'] = $_SESSION['theme']; }

foreach ($config as $k => $v) {
   define($k, $v);
}


define('DIR_SYSTEM', DIR_BASE . 'system/');
define('DIR_MODEL', DIR_BASE . 'model/');
define('DIR_DATABASE', DIR_BASE . 'system/database/');
define('DIR_IMAGE', DIR_BASE . 'image/');
define('DIR_LANGUAGE', DIR_BASE . 'language/');
define('DIR_APPLICATION', DIR_BASE . 'controller/');
define('DIR_TEMPLATE', DIR_BASE . 'view/theme/default/templates/');
define('DIR_CACHE', DIR_BASE . 'cache/');
define('DIR_REPORT', DIR_BASE . 'reports/');
define('DIR_LOG', DIR_BASE . 'log/');

define('QSHAPE_ACTIVE_INCOMING', STAT_DIRECTORY . '/active+incoming');
define('QSHAPE_ACTIVE_INCOMING_SENDER', STAT_DIRECTORY . '/active+incoming-sender');
define('QSHAPE_DEFERRED', STAT_DIRECTORY . '/deferred');
define('QSHAPE_DEFERRED_SENDER', STAT_DIRECTORY . '/deferred-sender');

define('QSHAPE_ACTIVE_INCOMING_OUT', QSHAPE_ACTIVE_INCOMING . '-out');
define('QSHAPE_ACTIVE_INCOMING_SENDER_OUT', QSHAPE_ACTIVE_INCOMING_SENDER . '-out');
define('QSHAPE_DEFERRED_OUT', QSHAPE_DEFERRED . '-out');
define('QSHAPE_DEFERRED_SENDER_OUT', QSHAPE_DEFERRED_SENDER . '-out');

define('CPUSTAT', STAT_DIRECTORY . '/cpu.stat');
define('MAILLOG_PID_FILE', '/var/run/clapf/clapf-maillog.pid');
define('AD_SYNC_STAT', STAT_DIRECTORY . '/adsync.stat');
define('DAILY_QUARANTINE_REPORT_STAT', STAT_DIRECTORY . '/daily-quarantine-report.stat');
define('LOCK_FILE', DIR_LOG . 'lock');
define('QRUNNER_LOCK_FILE', DIR_LOG . 'qrunner.lock');

define('EOL', "\n");

define('REMOTE_IMAGE_REPLACEMENT', WEBUI_DIRECTORY . 'view/theme/default/images/remote.gif');

define('TABLE_USER', 'user');
define('TABLE_EMAIL', 't_email');
define('TABLE_DOMAIN', 't_domain');
define('TABLE_MISC', 't_misc');
define('TABLE_WHITELIST', 't_white_list');
define('TABLE_BLACKLIST', 't_black_list');
define('TABLE_POLICY', 't_policy');
define('TABLE_REMOTE', 't_remote');
define('TABLE_SUMMARY', 'summary');
define('TABLE_COUNTERS', 't_counters');
define('TABLE_QUARANTINE_GROUP', 't_quarantine_group');
define('TABLE_QUARANTINE', 't_quarantine');
define('TABLE_SEARCH', 't_search');

define('SESSION_DATABASE', 'sessions/sessions.sdb');

define('HISTORY_WORKER_URL', SITE_URL . 'index.php?route=history/worker');
define('HISTORY_HELPER_URL', SITE_URL . 'index.php?route=history/helper');

define('HISTORY_REFRESH', 60);

define('HEALTH_WORKER_URL', SITE_URL . 'index.php?route=health/worker');
define('HEALTH_REFRESH', 60);                                             
define('HEALTH_RATIO', 80);

define('LOG_FILE', DIR_LOG . 'webui.log');

define('SIZE_X', 430);
define('SIZE_Y', 250);

define('DEFAULT_POLICY', 'default_policy');

$memcached_servers = array(         
      array('127.0.0.1', 11211)                                                                                                                                                          
                          );
$counters = array('_c:rcvd', '_c:mynetwork', '_c:ham', '_c:spam', '_c:possible_spam', '_c:unsure', '_c:minefield', '_c:virus', '_c:zombie', '_c:fp', '_c:fn', '_c:counters_last_update');                                                                

$health_smtp_servers = array( array(POSTFIX_LISTEN_ADDRESS, POSTFIX_LISTEN_PORT, "postfix before content-filter"), array(LOCALHOST, CLAPF_PORT, "content-filter"), array(LOCALHOST, POSTFIX_PORT_AFTER_CONTENT_FILTER, "postfix after content-filter") );

$partitions_to_monitor = array('/', '/home', '/var', '/tmp');

$postgrey_servers = array( );            

$langs = array(                     
                'hu',                                                                                                                                                                    
                'en'
               );


?>
