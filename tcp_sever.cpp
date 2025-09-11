#include "tcp_sever.h"
#include "camera.h"
#include "adc.h"
#include "hardware/pwm.h"

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
        // 受信したペイロードの先頭1バイトをコマンドとして解釈
        char cmd = ((char*)p->payload)[0];
        DEBUG_printf("Received cmd: '%c'\n", cmd);

        // 'g' (get) コマンドを受信したら画像データを送信
        if (cmd == 'g') {
            DEBUG_printf("Sending first line of frame_buffer (%d bytes)...\n", FRAME_WIDTH);
            
            // frame_bufferの1行目を送信
            // tcp_writeはバッファがいっぱいだと送信できない場合があるため、エラーチェックが重要
            err_t write_err = tcp_write(tpcb, &frame_buffer[0], FRAME_WIDTH, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                DEBUG_printf("Failed to write frame data, error: %d\n", write_err);
                return tcp_server_result(arg, -1);
            }
            // デバッグ目的として、frame_bufferの最初の16バイトを16進数で表示
            DEBUG_printf("Frame data (first 16 bytes): ");
            for (int i = 0; i < 16 && i < FRAME_WIDTH; i++) {
                    DEBUG_printf("%02X ", frame_buffer[i]);
            }
            DEBUG_printf("\n");

            // TCP送信バッファの内容をすぐに送信するよう指示
            tcp_output(tpcb);
        } else if (cmd == 'p') {
            float deg = light_deg();
            DEBUG_printf("Getting light_deg value: %.2f\n", deg);

            // float値を文字列に変換
            char response_buffer[32];
            int len = snprintf(response_buffer, sizeof(response_buffer), "%.2f", deg);
            
            // 変換した文字列をクライアントに送信
            err_t write_err = tcp_write(tpcb, response_buffer, len, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                DEBUG_printf("Failed to write light_deg data, error: %d\n", write_err);
                return tcp_server_result(arg, -1);
            }
            tcp_output(tpcb);
        } else {
            // 's' や 't' などのコマンドの場合は state に保存し、エコーバックする
            if (cmd == 's' || cmd == 't') {
                state->command = cmd;
            }
            
            DEBUG_printf("Echoing back received data.\n");
            err_t write_err = tcp_write(tpcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                DEBUG_printf("Failed to write data for echo, error: %d\n", write_err);
                return tcp_server_result(arg, -1);
            }
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

void run_echo_server(uint * slice_num, uint *channel, float deg) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    int counter = 0;
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

        // float goal = light_deg()-deg;
        // printf("goal=%f\n", goal);
        
        if (light_deg() >= -180-deg && light_deg() <= -90-deg) {
            pwm_set_chan_level(*slice_num, *channel,2300);// 1800
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("2300\n");
        }
        if (light_deg() >= -89-deg && light_deg() <= -45-deg) {
            pwm_set_chan_level(*slice_num, *channel,2100); //1700
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("2100\n");
        }
        if (light_deg() >= -44-deg && light_deg() <= -36-deg) {
            pwm_set_chan_level(*slice_num, *channel,1900); //1600
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("1600\n");
        }
        if (light_deg() >= -31-deg && light_deg() <= -35-deg) {
            pwm_set_chan_level(*slice_num, *channel,1600); //1500
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("1500\n");
        }
        // -20 to 20 is stop
        if (light_deg() >= -30-deg && light_deg() <= 30-deg) {
            pwm_set_chan_level(*slice_num, *channel,1500);
            // printf("1500\n");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            counter++;
            if (counter > 100) {
                
            }
        }
        if (light_deg() >= 31-deg && light_deg() <= 35-deg) {
            pwm_set_chan_level(*slice_num, *channel,1400); //1500
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("1400\n");
        }
        if (light_deg() >= 36-deg && light_deg() <= 45-deg) {
            pwm_set_chan_level(*slice_num, *channel,900); //1400
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("900\n");
        }
        if (light_deg() >= 46-deg && light_deg() <= 90-deg) {
            pwm_set_chan_level(*slice_num, *channel,800);// 1300
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("800\n"); 
        }
        if (light_deg() >= 91-deg && light_deg() <= 180-deg) {
            pwm_set_chan_level(*slice_num, *channel,700); //1200
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // printf("700\n");
        }
        
        state->command = 0;
        cyw43_arch_poll();
        sleep_ms(1); // CPU負荷を軽減
    }
    free(state);
}