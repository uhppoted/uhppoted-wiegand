#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#include <uart.h>
#include <wiegand.h>

#include "tcpd.h"

#define TCP_BUFFER_SIZE 2048

const uint16_t TCP_PORT = 4242;
const uint8_t TCP_POLL_INTERVAL = 5;
const uint8_t CR = 13;
const uint8_t LF = 10;

enum TCPD_STATE {
    TCPD_UNKNOWN = 0,
    TCPD_INITIALISED = 1,
    TCPD_CONNECTING = 2,
    TCPD_CONNECTED = 3,
    TCPD_FATAL = 4,
};

/* struct for TCPD state
 *
 */
struct {
    int link;
    enum TCPD_STATE state;
    bool initialised;
    bool listening;

} TCPD = {
    .link = -100000,
    .state = TCPD_UNKNOWN,
    .initialised = false,
    .listening = false,
};

const uint32_t TCPD_POLL = 1000;

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
    int idle_count;
} TCP_STATE;

void tcpd_listen();
err_t tcpd_close();
err_t tcpd_accept(void *, struct tcp_pcb *, err_t);
err_t tcpd_recv(void *, struct tcp_pcb *, struct pbuf *, err_t);
err_t tcpd_send(void *, struct tcp_pcb *, const char *);
err_t tcpd_sent(void *, struct tcp_pcb *, u16_t);
err_t tcpd_monitor(void *arg, struct tcp_pcb *tpcb);
void tcpd_err(void *, err_t);
err_t tcpd_result(int);
void tcpd_log(const char *);

void reply(void *context, const char *msg) {
    char s[64];
    err_t err;

    snprintf(s, sizeof(s), "%s\r\n", msg);

    if ((err = tcpd_send(context, TCP_STATE.client_pcb, s)) != ERR_OK) {
        snprintf(s, sizeof(s), "WIFI SEND ERROR (%d)", err);
        tcpd_log(s);
    }
}

/* Alarm handler for TCP poll timer.
 *
 */
int64_t tcpdi(alarm_id_t id, void *data) {
    uint32_t msg = MSG_TCPD_POLL;

    if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
        // nothing to do
    }

    return TCPD_POLL * 1000;
}

/* Initialises the TCP server.
 *
 */
bool tcpd_initialise(enum MODE mode) {
    char s[64];
    err_t err;

    if ((err = cyw43_arch_init()) != 0) {
        snprintf(s, sizeof(s), "WIFI INITIALISATION ERROR (%d)", err);
        tcpd_log(s);
        return false;
    }

    cyw43_arch_enable_sta_mode();

    if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
        snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
        tcpd_log(s);
        return false;
    }

    TCPD.state = TCPD_CONNECTING;

    if (!add_alarm_in_ms(TCPD_POLL, tcpdi, (void *)NULL, true)) {
        return false;
    }

    return true;
}

void tcpd_terminate() {
    cyw43_arch_deinit();
    tcpd_log("WIFI TERMINATED");
}

void tcpd_poll() {
    char s[64];
    err_t err;

    cyw43_arch_poll();

    int link = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

    if (TCPD.link != link) {
        TCPD.link = link;

        switch (link) {
        case CYW43_LINK_DOWN:
            tcpd_log("WIFI LINK DOWN");
            break;

        case CYW43_LINK_JOIN:
            if (TCPD.state != TCPD_CONNECTED) {
                tcpd_log("WIFI CONNECTED");
                break;

            case CYW43_LINK_FAIL:
                tcpd_log("WIFI CONNECTION FAILED");
                break;

            case CYW43_LINK_NONET:
                tcpd_log("WIFI SSID NOT FOUND");
                break;

            case CYW43_LINK_BADAUTH:
                tcpd_log("WIFI NOT AUTHORISED");
                break;

            case CYW43_LINK_NOIP:
                tcpd_log("WIFI NO IP");
                break;

            case CYW43_LINK_UP:
                tcpd_log("WIFI READY");
                break;

            default:
                tcpd_log("WIFI UNKNOWN STATE");
            }
        }
    }

    switch (link) {
    case CYW43_LINK_DOWN:
        if (TCPD.state != TCPD_CONNECTING) {
            if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
                snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
                tcpd_log(s);
            } else {
                tcpd_log("WIFI RECONNECTING");
                TCPD.state = TCPD_CONNECTING;
                TCPD.listening = false;
            }
        }
        break;

    case CYW43_LINK_JOIN:
        if (TCPD.state != TCPD_CONNECTED) {
            TCPD.state = TCPD_CONNECTED;
        }
        break;

    case CYW43_LINK_FAIL:
        if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
            snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
            tcpd_log(s);
        } else {
            tcpd_log("WIFI RECONNECTING");
            TCPD.listening = false;
            TCPD.state = TCPD_CONNECTING;
        }
        break;

    case CYW43_LINK_NONET:
        TCPD.listening = false;
        break;

    case CYW43_LINK_BADAUTH:
        TCPD.listening = false;
        TCPD.state = TCPD_FATAL;
        break;

    case CYW43_LINK_NOIP:
        break;

    case CYW43_LINK_UP:
        if (!TCPD.listening) {
            tcpd_listen();
        }
        break;
    }
}

void tcpd_listen() {
    char s[64];
    err_t err;

    snprintf(s, sizeof(s), "ADDRESS %s PORT %u", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    tcpd_log(s);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        tcpd_log("ERROR CREAING pcb");
        return;
    }

    if ((err = tcp_bind(pcb, NULL, TCP_PORT)) != 0) {
        snprintf(s, sizeof(s), "ERROR BINDING TO PORT %u (%d)", TCP_PORT, err);
        tcpd_log(s);
        return;
    }

    TCP_STATE.server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!TCP_STATE.server_pcb) {
        snprintf(s, sizeof(s), "LISTEN ERROR (%d)", err);
        tcpd_log(s);
        tcp_close(pcb);
        return;
    }

    tcp_arg(TCP_STATE.server_pcb, &TCP_STATE);
    tcp_accept(TCP_STATE.server_pcb, tcpd_accept);

    tcpd_log("LISTENING");
    TCPD.listening = true;
}

err_t tcpd_accept(void *arg, struct tcp_pcb *client, err_t err) {
    char s[64];
    snprintf(s, sizeof(s), "%s  INCOMING", ip4addr_ntoa(&client->remote_ip));
    tcpd_log(s);

    // ... accept error?
    if (err != ERR_OK || client == NULL) {
        snprintf(s, sizeof(s), "ACCEPT ERROR (%d)", err);
        tcpd_log(s);

        if (client != NULL) {
            err_t err = tcp_close(client);
            if (err != ERR_OK) {
                snprintf(s, sizeof(s), "CLOSE ERROR (%d)", err);
                tcpd_log(s);
                tcp_abort(client);
                return ERR_ABRT;
            }
        }

        return ERR_VAL;
    }

    // ... connected!
    snprintf(s, sizeof(s), "%s  CONNECTED", ip4addr_ntoa(&client->remote_ip));
    tcpd_log(s);

    TCP_STATE.client_pcb = client;
    TCP_STATE.idle_count = 0;

    tcp_arg(client, &TCP_STATE);
    tcp_sent(client, tcpd_sent);
    tcp_recv(client, tcpd_recv);
    tcp_poll(client, tcpd_monitor, TCP_POLL_INTERVAL * 2);
    tcp_err(client, tcpd_err);

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

            snprintf(s, sizeof(s), "CLOSE ERROR (%d)", err);
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

err_t tcpd_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    char s[64];

    // ... closed ?
    if (p == NULL) {
        snprintf(s, sizeof(s), "%s  CLOSED", ip4addr_ntoa(&pcb->remote_ip));
        tcpd_log(s);

        tcp_arg(TCP_STATE.client_pcb, NULL);
        tcp_poll(TCP_STATE.client_pcb, NULL, 0);
        tcp_sent(TCP_STATE.client_pcb, NULL);
        tcp_recv(TCP_STATE.client_pcb, NULL);
        tcp_err(TCP_STATE.client_pcb, NULL);

        return ERR_OK;
    }

    // cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        snprintf(s, sizeof(s), "WIFI RECV L:%d N:%d err:%d", p->tot_len, TCP_STATE.recv_len, err);
        tcpd_log(s);

        const uint16_t remaining = TCP_BUFFER_SIZE - TCP_STATE.recv_len;
        TCP_STATE.recv_len += pbuf_copy_partial(p,
                                                TCP_STATE.buffer_recv + TCP_STATE.recv_len,
                                                p->tot_len > remaining ? remaining : p->tot_len, 0);
        tcp_recved(pcb, p->tot_len);
    }

    pbuf_free(p);

    // ... overflow?
    if (TCP_STATE.recv_len == TCP_BUFFER_SIZE) {
        TCP_STATE.recv_len = 0;
    }

    // ... CRLF?
    for (int i = 0; i < TCP_STATE.recv_len; i++) {
        if (TCP_STATE.buffer_recv[i] == CR || TCP_STATE.buffer_recv[i] == LF) {
            char cmd[64];
            int N = i < 64 ? i + 1 : 64;
            snprintf(cmd, N, "%s", TCP_STATE.buffer_recv);

            TCP_STATE.recv_len = 0;

            execw(cmd, reply, arg);

            return ERR_OK;
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
        snprintf(s, sizeof(s), "SEND ERROR (%d)", err);
        tcpd_log(s);

        return err;
    }

    return ERR_OK;
}

// TODO dequeue next message
err_t tcpd_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    char s[64];
    snprintf(s, sizeof(s), "%s  SENT %d BYTES", ip4addr_ntoa(&pcb->remote_ip), len);
    tcpd_log(s);

    // TCP_STATE *state = (TCP_STATE *) arg;
    TCP_STATE.sent_len += len;

    if (TCP_STATE.sent_len >= TCP_BUFFER_SIZE) {
    }

    return ERR_OK;
}

err_t tcpd_monitor(void *arg, struct tcp_pcb *pcb) {
    char s[64];
    err_t err = ERR_OK;

    if (++TCP_STATE.idle_count > 12) {
        snprintf(s, sizeof(s), "%s  IDLE", ip4addr_ntoa(&pcb->remote_ip));
        tcpd_log(s);

        tcp_arg(TCP_STATE.client_pcb, NULL);
        tcp_poll(TCP_STATE.client_pcb, NULL, 0);
        tcp_sent(TCP_STATE.client_pcb, NULL);
        tcp_recv(TCP_STATE.client_pcb, NULL);
        tcp_err(TCP_STATE.client_pcb, NULL);

        if ((err = tcp_close(pcb)) != ERR_OK) {
            snprintf(s, sizeof(s), "%s  CLOSE ERROR (%d)", ip4addr_ntoa(&pcb->remote_ip), err);
            tcpd_log(s);
            tcp_abort(pcb);
            err = ERR_ABRT;
        }

        TCP_STATE.client_pcb = NULL;
    } else {
        snprintf(s, sizeof(s), "%s  OK", ip4addr_ntoa(&pcb->remote_ip));
        tcpd_log(s);
    }

    return err;
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

    snprintf(s, sizeof(s), "%-6s %s", "TCPD", msg);

    tx(s);
}
