/*
 * sig.h
 */

#include <signal.h>

void sig_block(int sig);
void sig_unblock(int sig);
void sig_catch(int sig, void (*f)());
void sig_pause();
int wait_nohang(int *wstat);

void (*sig_defaulthandler)() = SIG_DFL;

#define sig_uncatch(s) (sig_catch((s),sig_defaulthandler))

