#!/bin/bash

# Make the training through the folders by MUA.
# It's easier and faster but of course not so efficient.
# 
# It works without modification only with a postfixadmin installation.
# It trains messages to the global tokens database.
#
# 03.10.2010 - Initial release
#
# QUICK HOWTO
# 1. Set CLAPF_BASE.
# 2. Set MAILDIR_BAE .
# 3. Set training cron job.
# 4. Place a file called .spam_delivery in the home of the user (eg. ~/Maildir/../.spam_delivered).
# 5. Make sure spaminess_oblivion_limit >= spam_overall_limit.
# 6. Probably the users want to setup filter rules.


CLAPF_BASE="/usr"

PATH="${CLAPF_BASE}/bin:${CLAPF_BASE}/sbin:/root/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

# base directory for the mail pool on the FS
MAILDIR_BASE="/data/mail"

# list maildirs (postfixadmin style mysql scheme)
list_user_dirs () {
	 mysql d_mail -e 'select maildir from mailbox'|grep -vw ^maildir$;
	}

for user_dir in `list_user_dirs`;do
	if [ -f "${MAILDIR_BASE}"/"${user_dir}"/.spam_delivered ];then
			# do ham-to-spam* folders exist?
			if [ ! -d "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam.done ];then
				if [ ! -d "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam ];
				# if not, create them and make the subscription
				then
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam _dovecot
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam.done _dovecot
					echo clapf.ham-to-spam >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
					echo clapf.ham-to-spam.done >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
				# create only the .done folder
				else
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam.done _dovecot
					echo clapf.ham-to-spam.done >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
				fi
			fi

			# similar as above
			if [ ! -d "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham.done ];then
				if [ ! -d "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham.done ];
				then
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham _dovecot
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham.done _dovecot
					echo clapf.spam-to-ham >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
					echo clapf.spam-to-ham.done >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
				else
					maildirmake.dovecot "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham.done _dovecot
					echo clapf.ham-to-spam.done >> "${MAILDIR_BASE}"/"${user_dir}"/Maildir/subscriptions
				fi
			fi
				
		# HAM -> SPAM
		for spam_email in `ls "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam/cur/`;do
			spamdrop -S < "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam/cur/"${spam_email}";
			mv "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam/cur/"${spam_email}" "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.ham-to-spam.done/cur/
			echo $spam_email
		done

		# SPAM -> HAM
		for ham_email in `ls "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham/cur/`;do
			spamdrop -H < "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham/cur/"${ham_email}";
			mv "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham/cur/"${ham_email}" "${MAILDIR_BASE}"/"${user_dir}"/Maildir/.clapf.spam-to-ham.done/cur/
			echo $ham_email
		done
	fi
done
