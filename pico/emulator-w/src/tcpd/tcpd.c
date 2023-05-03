#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#include "uart.h"

#include "tcpd.h"

#define TCP_BUFFER_SIZE 2048
#define TCP_PORT 4242
#define CR 13
#define LF 10

/* Internal state for the TCP server.
 *
 */
struct {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_sent[TCP_BUFFER_SIZE];
    uint8_t buffer_recv[TCP_BUFFER_SIZE];
    int sent_len;
    int recv_len;
    int run_count;
} TCP_STATE;

void tcpd_run();
bool tcpd_open();
err_t tcpd_close();
err_t tcpd_accept(void *, struct tcp_pcb *, err_t);
err_t tcpd_recv(void *, struct tcp_pcb *, struct pbuf *, err_t);
err_t tcpd_send(void *, struct tcp_pcb *, const char *);
void tcpd_err(void *, err_t);
err_t tcpd_result(int);
void tcpd_log(const char *);

/* Initialises the TCP server.
 *
 */
void tcpd_initialise(enum MODE mode) {
    if (cyw43_arch_init()) {
        tcpd_log("WIFI INITIALISATION FAILED");
    } else {
        tcpd_log("WIFI INITIALISED");

        cyw43_arch_enable_sta_mode();

        if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
            tcpd_log("WIFI CONNECT FAILED");
        } else {
            tcpd_log("WIFI CONNECTED");
            tcpd_run();
        }
    }
}

void tcpd_terminate() {
    cyw43_arch_deinit();
    tcpd_log("WIFI TERMINATED");
}

void tcpd_run() {
    if (!tcpd_open()) {
        tcpd_log("OPEN ERROR");
        tcpd_result(-1);
        return;
    }

    //     while(!state->complete) {
    //         // the following #ifdef is only here so this same example can be used in multiple modes;
    //         // you do not need it in your code
    // #if PICO_CYW43_ARCH_POLL
    //         // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
    //         // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
    //         cyw43_arch_poll();
    //         // you can poll as often as you like, however if you have nothing else to do you can
    //         // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
    //         cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    // #else
    //         // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
    //         // is done via interrupt in the background. This sleep is just an example of some (blocking)
    //         // work you might be doing.
    //         sleep_ms(1000);
    // #endif
    //     }
}

bool tcpd_open() {
    char s[64];

    snprintf(s, sizeof(s), "ADDRESS %s PORT %u", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    tcpd_log(s);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        tcpd_log("ERROR CREAING pcb");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        snprintf(s, sizeof(s), "ERROR BINDING TO PORT %u", TCP_PORT);
        tcpd_log(s);
        return false;
    }

    TCP_STATE.server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!TCP_STATE.server_pcb) {
        tcpd_log("LISTEN ERROR");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(TCP_STATE.server_pcb, &TCP_STATE);
    tcp_accept(TCP_STATE.server_pcb, tcpd_accept);

    tcpd_log("LISTENING");

    return true;
}

err_t tcpd_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    tcpd_log("INCOMING");

    if (err != ERR_OK || client_pcb == NULL) {
        tcpd_log("ACCEPT ERROR");
        tcpd_result(err);
        return ERR_VAL;
    }

    tcpd_log("CLIENT CONNECTED");

    TCP_STATE.client_pcb = client_pcb;
    tcp_arg(client_pcb, &TCP_STATE);
    // tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcpd_recv);
    // tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcpd_err);

    // return tcp_server_send_data(arg, state->client_pcb);

    return ERR_OK;
}

err_t tcpd_close() {
    err_t err = ERR_OK;

    if (TCP_STATE.client_pcb != NULL) {
        tcp_arg(TCP_STATE.client_pcb, NULL);
        tcp_poll(TCP_STATE.client_pcb, NULL, 0);
        tcp_sent(TCP_STATE.client_pcb, NULL);
        tcp_recv(TCP_STATE.client_pcb, NULL);
        tcp_err(TCP_STATE.client_pcb, NULL);

        err = tcp_close(TCP_STATE.client_pcb);
        if (err != ERR_OK) {
            char s[64];

            snprintf(s, sizeof(s), "CLOSE ERROR %d, calling abort", err);
            tcpd_log(s);
            tcp_abort(TCP_STATE.client_pcb);
            err = ERR_ABRT;
        }

        TCP_STATE.client_pcb = NULL;
    }

    if (TCP_STATE.server_pcb) {
        tcp_arg(TCP_STATE.server_pcb, NULL);
        tcp_close(TCP_STATE.server_pcb);
        TCP_STATE.server_pcb = NULL;
    }

    return err;
}

err_t tcpd_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        return tcpd_result(-1);
    }

    // cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        char s[64];
        snprintf(s, sizeof(s), "RECV L:%d  N:%d  err:%d", p->tot_len, TCP_STATE.recv_len, err);
        tcpd_log(s);

        const uint16_t remaining = TCP_BUFFER_SIZE - TCP_STATE.recv_len;
        TCP_STATE.recv_len += pbuf_copy_partial(p,
                                                TCP_STATE.buffer_recv + TCP_STATE.recv_len,
                                                p->tot_len > remaining ? remaining : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }

    pbuf_free(p);

    // ... overflow?
    if (TCP_STATE.recv_len == TCP_BUFFER_SIZE) {
        TCP_STATE.recv_len = 0;
    }

    // ... CRLF?
    for (int i = 0; i < TCP_STATE.recv_len; i++) {
        if (TCP_STATE.buffer_recv[i] == CR || TCP_STATE.buffer_recv[i] == LF) {
            char s[64];
            int N = i < 64 ? i + 1 : 64;
            snprintf(s, N, "%s", TCP_STATE.buffer_recv);
            tcpd_log(s);

            TCP_STATE.recv_len = 0;

            return tcpd_send(arg, TCP_STATE.client_pcb, "wootedy woot woot\r\n");
        }
    }

    return ERR_OK;
}

err_t tcpd_send(void *arg, struct tcp_pcb *tpcb, const char *msg) {
    int N = strlen(msg);
    int ix = 0;
    int i = 0;

    while (ix < TCP_BUFFER_SIZE && i < N) {
        TCP_STATE.buffer_sent[ix++] = msg[i++];
    }

    TCP_STATE.sent_len = 0;

    // cyw43_arch_lwip_check();

    err_t err = tcp_write(tpcb, TCP_STATE.buffer_sent, i, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        char s[64];
        snprintf(s, sizeof(s), "SEND ERROR %d", err);
        tcpd_log(s);

        return tcpd_result(-1);
    }

    return ERR_OK;
}

void tcpd_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        char s[64];

        snprintf(s, sizeof(s), "ERROR %d", err);
        tcpd_log(s);
        tcpd_result(err);
    }
}

err_t tcpd_result(int status) {
    if (status == 0) {
        tcpd_log("OK");
    } else {
        tcpd_log("FAILED");
    }

    TCP_STATE.complete = true;

    return tcpd_close();
}

void tcpd_log(const char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%-6s %s", "TCP", msg);

    tx(s);
}
