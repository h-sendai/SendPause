/*
Pause frame
+---------+--------+---------+-------+--------------+---------+--------+
| dest(6) | src(6) | type(2) | op(2) | pausetime(2) | pad(42) | FCS(4) |
+---------+--------+---------+-------+--------------+---------+--------+
                             |<----- Ethernet Payload ------> |
dest:      01:80:C2:00:00:01
type:      0x8808
op code:   0x0001
pausetime: 0 - 65535
*/

/*
 * MicroMiniHowto: 
 * Send an arbitrary Ethernet frame using an AF_PACKET socket in C
 */

/*
 * 1. Select the required EtherType.
 * 2. Create the AF_PACKET socket (see packet(7)).
 * 3. Determine the index number of the Ethernet interface to be used.
 *    (use interface name such as eth0, eno1, enp0s8 etc.)
 * 4. Construct the destination address.
 * 5. Send the Ethernet frame.
 */

#include "flow_ctrl_pause.h"

/*
 * 3. Determine the index number of the Ethernet interface to be used.
 *    (use interface name such as eth0, eno1, enp0s8 etc.)
 *    If we cannot use if_nametoindex(), use following get_en_index()
 */
#if 0
static int get_en_index(int fd, char *if_name)
{
    struct ifreq ifr; /* netdevice(7) */

    size_t if_name_len = strlen(if_name);
    if (if_name_len < sizeof(ifr.ifr_name)) {
        memcpy(ifr.ifr_name, if_name, if_name_len);
        ifr.ifr_name[if_name_len] = 0;
    }
    else {
        warnx("interfacename too long: %s", if_name);
        return -1;
    }
    
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        warn("ioctl(fd, SIOCGIFINDEX, &ifr) for %s", if_name);
        return -1;
    }

    return ifr.ifr_ifindex;
}
#endif

int send_flow_ctrl_pause(char *if_name, int pause_time)
{
    int i;
    
    if (pause_time < 0 || pause_time > 65535) {
        warn("too large pause_time: %d", pause_time);
        return -1;
    }

    /* 2. Create the AF_PACKET socket (see packet(7)). */
    int fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_PAUSE));
    if (fd < 0) {
        warn("socket");
        return -1;
    }

    /* 3. Determine the index number of the Ethernet interface to be used. */
    unsigned int if_index;
    if ( (if_index = if_nametoindex(if_name)) == 0) {
        warn("if_nametoindex");
        return -1;
    }
    
    /* 4. Construct the destination address */
    unsigned char ether_pause_addr[] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x01 };
    struct sockaddr_ll addr = { 0 };
    addr.sll_family   = AF_PACKET;
    addr.sll_ifindex  = if_index;
    addr.sll_halen    = ETHER_ADDR_LEN;
    addr.sll_protocol = htons(ETH_P_PAUSE);
    memcpy(addr.sll_addr, ether_pause_addr, ETHER_ADDR_LEN);

    unsigned char en_payload[46];
    en_payload[0] = 0x00;
    en_payload[1] = 0x01;
    en_payload[2] = pause_time >> 8;
    en_payload[3] = pause_time;
    for (i = 4; i < 46; ++i) {
        en_payload[i] = 0xff; /* padding */
    }

    /* 5. Send the Ethernet frame. */
    if (sendto(fd, en_payload, sizeof(en_payload), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        warn("sendto");
        return -1;
    }

    if (close(fd) < 0) {
        warn("close");
        return -1;
    }

    return 0;
}

#ifdef USE_MAIN
int main(int argc, char *argv[])
{
    char *if_name;
    int pause_time;

    if (argc != 3) {
        fprintf(stderr, "Usage: sample if_name pause_time (0 - 65535)\n");
        exit(1);
    }
    if_name    = argv[1];
    pause_time = strtol(argv[2], NULL, 0);

    uid_t uid = geteuid();
    if (uid != 0) {
        fprintf(stderr, "root priv. required.  Use sudo or su\n");
        exit(1);
    }

    if (send_flow_ctrl_pause(if_name, pause_time) < 0) {
        fprintf(stderr, "flow_ctrl_pause() error\n");
    }

    return 0;
}
#endif
