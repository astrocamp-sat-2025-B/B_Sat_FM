#include "tcp_sever.h"
#include "camera.h"
#include "adc.h"

#define TCP_PORT 4242
#define DEBUG_printf printf

// 関数のプロトタイプ宣言
static err_t tcp_send_chunk(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    state->command = 0;
    state->send_buffer_ptr = NULL;
    state->send_buffer_len = 0;
    state->send_buffer_pos = 0;
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

static err_t tcp_send_chunk(void *arg, struct tcp_pcb *tpcb) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    if (state->send_buffer_ptr == NULL || state->send_buffer_pos >= state->send_buffer_len) {
        return ERR_OK;
    }

    u16_t remaining = state->send_buffer_len - state->send_buffer_pos;
    u16_t send_len = tcp_sndbuf(tpcb);

    if (send_len == 0) {
        return ERR_OK;
    }

    send_len = (send_len < remaining) ? send_len : remaining;

    err_t write_err = tcp_write(tpcb, state->send_buffer_ptr + state->send_buffer_pos, send_len, TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        DEBUG_printf("Failed to write chunk data, error: %d\n", write_err);
        return tcp_server_close(arg);
    }
    
    state->send_buffer_pos += send_len;
    DEBUG_printf("Sent chunk of %u bytes, total sent: %u/%u\n", send_len, state->send_buffer_pos, state->send_buffer_len);

    if (state->send_buffer_pos >= state->send_buffer_len) {
        DEBUG_printf("Finished sending full image buffer.\n");
        state->send_buffer_ptr = NULL;
    }
    
    tcp_output(tpcb);

    return ERR_OK;
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    return tcp_send_chunk(arg, tpcb);
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        DEBUG_printf("Client disconnected\n");
        return tcp_server_close(arg);
    }
    
    cyw43_arch_lwip_check();

    // ★ 修正点2: 構造を修正。ifブロックの外にクリーンアップ処理を移動
    if (p->tot_len > 0) {
        char cmd = ((char*)p->payload)[0];
        DEBUG_printf("Received cmd: '%c'\n", cmd);

        if (cmd == 'g') {
            const uint32_t total_size = FRAME_HEIGHT * FRAME_WIDTH;
            DEBUG_printf("Preparing to send full frame_buffer (size: %u bytes)...\n", total_size);
            for (int y = 0; y < FRAME_HEIGHT; y++) {
                for (int x = 0; x < FRAME_WIDTH * 2; x++) {
                    DEBUG_printf("%02X ", frame_buffer[y * FRAME_WIDTH + x]);
                }
                DEBUG_printf("\n");
            }
            state->send_buffer_ptr = frame_buffer;
            state->send_buffer_len = total_size;
            state->send_buffer_pos = 0;
            tcp_send_chunk(arg, tpcb);

        } else if (cmd == 'p') {
            float deg = light_deg();
            DEBUG_printf("Getting light_deg value: %.2f\n", deg);
            char response_buffer[32];
            int len = snprintf(response_buffer, sizeof(response_buffer), "%.2f", deg);
            
            err_t write_err = tcp_write(tpcb, response_buffer, len, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                return tcp_server_result(arg, -1);
            }
            tcp_output(tpcb);

        } else {
            if (cmd == 's' || cmd == 't') {
                state->command = cmd;
            }
            
            DEBUG_printf("Echoing back received data.\n");
            err_t write_err = tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                return tcp_server_result(arg, -1);
            }
            tcp_output(tpcb);
        }
    }

    tcp_recved(tpcb, p->tot_len);
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
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

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
        cyw43_arch_poll();
        sleep_ms(1);
    }
    free(state);
}