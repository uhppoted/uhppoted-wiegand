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
    uint8_t buffer_sent[TCP_BUFFER_SIZE];
    uint8_t buffer_recv[TCP_BUFFER_SIZE];
    int sent_len;
    int recv_len;
} TCP_STATE;

/* State for a client connection.
 *
 */
typedef struct connection {
    struct tcp_pcb *server;
    struct tcp_pcb *client;
    uint8_t buffer_sent[TCP_BUFFER_SIZE];
    uint8_t buffer_recv[TCP_BUFFER_SIZE];
    int sent;
    int received;
    int idle;
} connection;

/** Function prototypes **/

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
    connection *conn = (connection *)context;
    conn->idle = 0;

    char s[64];
    err_t err;

    snprintf(s, sizeof(s), "%s\r\n", msg);

    if ((err = tcpd_send(context, conn->client, s)) != ERR_OK) {
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
    snprintf(s, sizeof(s), "%s:%d  INCOMING", ip4addr_ntoa(&client->remote_ip), client->remote_port);
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
    snprintf(s, sizeof(s), "%s:%d  CONNECTED", ip4addr_ntoa(&client->remote_ip), client->remote_port);
    tcpd_log(s);

    connection *conn = (connection *)malloc(sizeof(connection));

    conn->server = TCP_STATE.server_pcb;
    conn->client = client;
    conn->sent = 0;
    conn->received = 0;
    conn->idle = 0;

    tcp_arg(client, conn);
    tcp_sent(client, tcpd_sent);
    tcp_recv(client, tcpd_recv);
    tcp_poll(client, tcpd_monitor, TCP_POLL_INTERVAL * 2);
    tcp_err(client, tcpd_err);

    return ERR_OK;
}

err_t tcpd_close() {
    err_t err = ERR_OK;

    struct tcp_pcb *p = TCP_STATE.client_pcb;

    // FIXME free connection structs
    while (p != NULL) {
        tcp_arg(p, NULL);
        tcp_poll(p, NULL, 0);
        tcp_sent(p, NULL);
        tcp_recv(p, NULL);
        tcp_err(p, NULL);

        if ((err = tcp_close(p)) != ERR_OK) {
            char s[64];

            snprintf(s, sizeof(s), "CLOSE ERROR (%d)", err);
            tcpd_log(s);
            tcp_abort(p);
            err = ERR_ABRT;
        }

        p = p->next;
    }

    TCP_STATE.client_pcb = NULL;

    if (TCP_STATE.server_pcb) {
        tcp_arg(TCP_STATE.server_pcb, NULL);
        tcp_close(TCP_STATE.server_pcb);
        TCP_STATE.server_pcb = NULL;
    }

    return err;
}

err_t tcpd_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    char s[64];

    snprintf(s, sizeof(s), ">>>> DEBUG state:%p this:%p  next:%p", TCP_STATE.client_pcb, pcb, pcb->next);
    tcpd_log(s);

    connection *conn = (connection *)arg;
    conn->idle = 0;

    // ... closed ?
    if (p == NULL) {
        snprintf(s, sizeof(s), "%s:%d  CLOSED", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_log(s);

        tcp_arg(conn->client, NULL);
        tcp_poll(conn->client, NULL, 0);
        tcp_sent(conn->client, NULL);
        tcp_recv(conn->client, NULL);
        tcp_err(conn->client, NULL);

        free(conn);

        return ERR_OK;
    }

    // cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        snprintf(s, sizeof(s), "WIFI RECV L:%d N:%d err:%d", p->tot_len, conn->received, err);
        tcpd_log(s);

        const uint16_t remaining = TCP_BUFFER_SIZE - conn->received;
        conn->received += pbuf_copy_partial(p,
                                            conn->buffer_recv + conn->received,
                                            p->tot_len > remaining ? remaining : p->tot_len, 0);
        tcp_recved(pcb, p->tot_len);
    }

    pbuf_free(p);

    // ... overflow?
    if (conn->received == TCP_BUFFER_SIZE) {
        conn->received = 0;
    }

    // ... CRLF?
    for (int i = 0; i < conn->received; i++) {
        if (conn->buffer_recv[i] == CR || conn->buffer_recv[i] == LF) {
            char cmd[64];
            int N = i < 64 ? i + 1 : 64;
            snprintf(cmd, N, "%s", conn->buffer_recv);

            conn->received = 0;

            execw(cmd, reply, arg);

            return ERR_OK;
        }
    }

    return ERR_OK;
}

err_t tcpd_send(void *arg, struct tcp_pcb *pcb, const char *msg) {
    int N = strlen(msg);
    int ix = 0;
    int i = 0;

    connection *conn = (connection *)arg;
    conn->idle = 0;

    while (ix < TCP_BUFFER_SIZE && i < N) {
        conn->buffer_sent[ix++] = msg[i++];
    }

    conn->sent = 0;

    // cyw43_arch_lwip_check();

    err_t err = tcp_write(pcb, conn->buffer_sent, i, TCP_WRITE_FLAG_COPY);
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
    snprintf(s, sizeof(s), "%s:%d  SENT %d BYTES", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, len);
    tcpd_log(s);

    connection *conn = (connection *)arg;
    conn->idle = 0;

    conn->sent += len;

    if (conn->sent >= TCP_BUFFER_SIZE) {
    }

    return ERR_OK;
}

err_t tcpd_monitor(void *arg, struct tcp_pcb *pcb) {
    char s[64];
    err_t err = ERR_OK;

    connection *conn = (connection *)arg;

    conn->idle++;

    if (conn->idle > 12) {
        snprintf(s, sizeof(s), "%s:%d  IDLE", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_log(s);

        tcp_arg(conn->client, NULL);
        tcp_poll(conn->client, NULL, 0);
        tcp_sent(conn->client, NULL);
        tcp_recv(conn->client, NULL);
        tcp_err(conn->client, NULL);

        free(conn);

        if ((err = tcp_close(pcb)) != ERR_OK) {
            snprintf(s, sizeof(s), "%s:%d  CLOSE ERROR (%d)", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, err);
            tcpd_log(s);
            tcp_abort(pcb);
            err = ERR_ABRT;
        }

    } else {
        snprintf(s, sizeof(s), "%s:%d  OK", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
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

    return tcpd_close();
}

void tcpd_log(const char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%-6s %s", "TCPD", msg);

    tx(s);
}
