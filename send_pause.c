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
int sockfd     = -1;
int pause_time = 65535;
int quiet      = 0;
int debug      = 0;

int usage(void)
{
    char msg[] = "Usage: send_pause [-c count] [-p pause_time] [-i interval_sec] if_name\n"
                 "Arguments:\n"
                 "if_name (mandatory): NIC name such as eth0, eno1, enp0s6.\n"
                 "-c count:            Exit after send count pause packets.  Default no limit.\n"
                 "-p pause_time:       pause time (0 - 65535).  Default 65535 (max).\n"
                 "-i interval_sec:     Default 1 second.  You may use float number (1.5 for example)\n"
                 "\n"
                 "Hint: You may use $((2**15)) on the shell command line to caculate 2^15\n"
                 "pause_v   decimal     512 bt   1GbE ms\n"
                 "--------  -------  ---------  --------\n"
                 "2**32-1     65535   33553920      33.5\n"
                 "2**15       32768   16777216      16.7\n"
                 "2**14       16384    8388608       8.3\n"
                 "2**13        8192    4194304       4.1\n"
                 "2**12        4096    2097152       2.0\n";

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
    if (debug) {
        fprintfwt(stdout, "try to send pause to %s, pause time: %d\n", if_name, pause_time);
    }

    if (send_pause_packet(sockfd, if_name, pause_time) < 0) {
         exit(1);
         // errx(1, "flow_ctrl_pause() error");
    }
    if (! quiet) {
        fprintfwt(stdout, "send pause to %s, pause time: %d\n", if_name, pause_time);
    }

    return;
}

int main(int argc, char *argv[])
{
    struct timeval interval = { 1, 0 };
    int count               = 0;
    int max_count           = 0;

    int c;

    while ( (c = getopt(argc, argv, "c:di:p:q")) != -1) {
        switch (c) {
            case 'c':
                max_count = strtol(optarg, NULL, 0);
                break;
            case 'd':
                debug = 1;
                break;
            case 'i':
                interval = str2timeval(optarg);
                break;
            case 'p':
                pause_time = strtol(optarg, NULL, 0);
                break;
            case 'q':
                quiet = 1;
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
        exit(1);
    }

    if_name = argv[0];
    sockfd  = create_pause_socket();

    my_signal(SIGALRM, send_pause);
    set_timer(interval.tv_sec, interval.tv_usec, interval.tv_sec, interval.tv_usec);

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
    
    if (close(sockfd) < 0) {
        err(1, "close");
    }

    return 0;
}
