// tcp_server.h

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    volatile char command;
} TCP_SERVER_T;

void run_echo_server(void);

#endif // TCP_SERVER_H