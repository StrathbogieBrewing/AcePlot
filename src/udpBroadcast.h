#ifndef UDPBROADCAST_H
#define UDPBROADCAST_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

int udpBroadcast_open(int udp_port);
int udpBroadcast_send(unsigned char *data, int bytes);
void udpBroadcast_close(void);

#if defined(__cplusplus)
}
#endif

#endif // UDPBROADCAST_H
