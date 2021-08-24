#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "udpBroadcast.h"

static int udp_socket = -1;
static struct sockaddr_in si_me;
static struct sockaddr_in si_broadcast;

int udpBroadcast_open(int udp_port) {
  // check socket doesnt already exist
  if (udp_socket != -1) {
    fprintf(stderr, "UDP Socket already open\n");
    return -1;
  }

  // create a non blocking socket
  udp_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
  if (udp_socket == -1) {
    fprintf(stderr, "UDP Socket could not be opened\n");
    return -1;
  }

  int broadcast = 1;
  int err = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast,
                       sizeof(broadcast));
  if (err == -1) {
    fprintf(stderr, "UDP Socket could not be set for broadcast\n");
    return -1;
  }

  int reuse = 1;
  err = setsockopt(udp_socket, SOL_SOCKET, SO_REUSEPORT, &reuse,
                       sizeof(reuse));
  if (err == -1) {
    fprintf(stderr, "UDP Socket could not be set for port reuse\n");
    return -1;
  }

  // broadcast
  memset(&si_broadcast, 0, sizeof(si_broadcast));
  si_broadcast.sin_family = AF_INET;
  si_broadcast.sin_addr.s_addr = inet_addr("255.255.255.255");
  si_broadcast.sin_port = htons(udp_port);
}

int udpBroadcast_send(unsigned char *data, int bytes) {
  if (udp_socket == -1) {
    fprintf(stderr, "UDP Socket not open\n");
    return -1;
  }
  return sendto(udp_socket, data, bytes, 0, (struct sockaddr *)&si_broadcast,
                sizeof(si_broadcast));
}

void udpBroadcast_close(void) {
  // check socket exists before closing
  if (udp_socket != -1) {
    close(udp_socket);
  }
}
