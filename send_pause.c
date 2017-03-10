#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "set_timer.h"
#include "my_signal.h"
#include "logUtil.h"
#include "flow_ctrl_pause.h"

char *if_name  = NULL;
int pause_time = 65535;
int quiet      = 0;
int debug      = 0;

int usage(void)
{
    char msg[] = "Usage: send_pause if_name [count [pause_time [interval_sec [interval_usec]]]]\n"
                 "Arguments:\n"
                 "if_name:       NIC name such as eth0, eno1, enp0s6.\n"
                 "count:         number of send pause packet.  0 means forever.\n"
                 "pause_time:    pause time (0 - 65535).  Default 65535 (max).\n"
                 "interval_sec:  default 1 second.\n"
                 "interval_usec: micro second.\n"
                 "Sample command:\n"
                 "send_pause eth0          send pause (pause_time 65535) packet every 1 second forever\n"
                 "send_pause eth0 10       send pause (pause_time 65535) packet every 1 second 10 times\n"
                 "send_pause eth0 10 32768 send pause (pause_time 32768) packet every 1 second 10 times\n"
                 "send_pause eth0  0 32768 send pause (pause_time 32768) packet every 1 second forever\n"
                 "Hint: You may use $((2**15)) on the shell command line to caculate 2^15\n";

    fprintf(stderr, "%s", msg);

    return 0;
}

/* for alarm signal handler debug */
void send_pause_print(int signo)
{
    fprintf(stderr, "flow_ctrl_pause(%s, 01:80:c2:00:00:01, %d)\n",
        if_name, pause_time);

    return;
}

void send_pause(int signo)
{
    if (! quiet) {
        fprintfwt(stdout, "send pause to %s, pause time: %d\n", if_name, pause_time);
    }
    if (send_flow_ctrl_pause(if_name, pause_time) < 0) {
         exit(1);
         // errx(1, "flow_ctrl_pause() error");
    }

    return;
}

int main(int argc, char *argv[])
{
    int interval_sec  = 1;
    int interval_usec = 0;
    int count         = 0;
    int max_count     = 0;

    int c;

    while ( (c = getopt(argc, argv, "dq")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'q':
                quiet = 1;
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0 || argc > 5) {
        usage();
        exit(1);
    }

    if (argc > 0) { /* one argument as "send_pause eth0" */
        if_name = argv[0];
    }
    if (argc > 1) {
        max_count = strtol(argv[1], NULL, 0);
    }
    if (argc > 2) {
        pause_time = strtol(argv[2], NULL, 0);
    }
    if (argc > 3) {
        interval_sec = strtol(argv[3], NULL, 0);
    }
    if (argc > 4) {
        interval_usec = strtol(argv[4], NULL, 0);
    }

    my_signal(SIGALRM, send_pause);
    set_timer(interval_sec, interval_usec, interval_sec, interval_usec);

    /* send first pause packet */
    send_pause(SIGALRM);
    count ++;

    for ( ; ; ) {
        if (count == max_count) {
            break;
        }
        else {
            pause();
            count ++;
        }
    }

    return 0;
}
