#include "tcp_sever.h"

#define TCP_PORT 4242
#define DEBUG_printf printf

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    state->command = 0;
    return state;
}

static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

static err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_server_close(arg);
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    DEBUG_printf("Sent %u bytes of data\n", len);
    return ERR_OK;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        DEBUG_printf("Client disconnected\n");
        return tcp_server_close(arg);
    }
    
    cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        char data = ((char*)p->payload)[0];
        DEBUG_printf("Received cmd: %c\n", data);

        if (data == 's'|| data == 'g' || data == 't')
        {
            state->command = data;
        }

        err_t write_err = tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
        if (write_err != ERR_OK) {
            DEBUG_printf("Failed to write data for echo, error: %d\n", write_err);
            return tcp_server_result(arg, -1);
        }

        // 改行文字を送信
        write_err = tcp_write(tpcb, "\n", 1, TCP_WRITE_FLAG_COPY);
        if (write_err != ERR_OK) {
            DEBUG_printf("Failed to write newline, error: %d\n", write_err);
            return tcp_server_result(arg, -1);
        }
    }

    // 受信処理が完了したことをTCPスタックに通知
    tcp_recved(tpcb, p->tot_len);
    // pbufを解放
    pbuf_free(p);

    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

static bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    // acceptコールバックを登録
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void run_echo_server(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    while(!state->complete) {
        switch (state->command)
        {
            case 's':
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                break; 

            case 'g':
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                break;
            
            default:
                break;
        }
        state->command = 0;
        cyw43_arch_poll();
        sleep_ms(1); // CPU負荷を軽減
    }
    free(state);
}