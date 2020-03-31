#ifndef _JOYLINK_SOFTAP_START_H_
#define _JOYLINK_SOFTAP_START_H_

int joylink_udp_broad_send(int socket_fd, char *buf, int len);

int joylink_softap_socket_send(int socket_fd, char *buf, int len);

int joylink_softap_start();

#endif
