#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#include <uart.h>
#include <wiegand.h>

#include "tcpd.h"

#define TCP_BUFFER_SIZE 512

const uint8_t CR = 13;
const uint8_t LF = 10;
const uint16_t TCP_PORT = 4242;
const uint32_t TCPD_POLL = 1000;
const uint8_t TCP_SERVER_POLL = 30;  // seconds
const uint8_t TCP_CLIENT_POLL = 5;   // seconds
const int CLIENT_IDLE_TIME = 2 * 60; // seconds
const int CLIENT_IDLE_COUNT = CLIENT_IDLE_TIME / TCP_CLIENT_POLL;
const int SERVER_IDLE_TIME = 5 * 60; // seconds
const int SERVER_IDLE_COUNT = SERVER_IDLE_TIME / TCP_SERVER_POLL;

enum TCPD_STATE {
    TCPD_UNKNOWN = 0,
    TCPD_INITIALISED = 1,
    TCPD_CONNECTING = 2,
    TCPD_CONNECTED = 3,
    TCPD_FATAL = 4,
};

/* Internal state for the TCP server.
 *
 */
typedef struct TCPD {
    int link;
    enum TCPD_STATE state;
    bool listening;
    bool closed;
    uint32_t idle;
    struct tcp_pcb *server;
} TCPD;

/* State for a client connection.
 *
 */
typedef struct connection {
    struct tcp_pcb *server;
    struct tcp_pcb *client;
    int sent;
    int received;
    int idle;
    uint8_t buffer_sent[TCP_BUFFER_SIZE];
    uint8_t buffer_recv[TCP_BUFFER_SIZE];
} connection;

static TCPD tcpd = {
    .link = -100000,
    .state = TCPD_UNKNOWN,
    .listening = false,
    .closed = false,
    .idle = 0,
};

static connection connections[2] = {
    {.server = NULL, .client = NULL, .sent = 0, .received = 0, .idle = 0},
    {.server = NULL, .client = NULL, .sent = 0, .received = 0, .idle = 0},
};

const int CONNECTIONS = sizeof(connections) / sizeof(connection);

/** Function prototypes **/

void tcpd_listen();
err_t tcpd_close();
err_t tcpd_accept(void *, struct tcp_pcb *, err_t);
err_t tcpd_recv(void *, struct tcp_pcb *, struct pbuf *, err_t);
err_t tcpd_send(void *, struct tcp_pcb *, const char *);
err_t tcpd_sent(void *, struct tcp_pcb *, u16_t);
err_t tcpd_server_monitor(void *, struct tcp_pcb *);
err_t tcpd_client_monitor(void *, struct tcp_pcb *);
void tcpd_err(void *, err_t);
void tcpd_log(const char *);

void reply(void *context, const char *msg) {
    char s[64];
    err_t err;

    connection *conn = (connection *)context;
    conn->idle = 0;

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

    tcpd.state = TCPD_CONNECTING;

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

    if (tcpd.link != link) {
        tcpd.link = link;

        switch (link) {
        case CYW43_LINK_DOWN:
            tcpd_log("WIFI LINK DOWN");
            break;

        case CYW43_LINK_JOIN:
            if (tcpd.state != TCPD_CONNECTED) {
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
        if (tcpd.state != TCPD_CONNECTING) {
            if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
                snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
                tcpd_log(s);
            } else {
                tcpd_log("WIFI RECONNECTING");
                tcpd.state = TCPD_CONNECTING;
                tcpd.listening = false;
                tcpd.closed = false;
            }
        }
        break;

    case CYW43_LINK_JOIN:
        if (tcpd.state != TCPD_CONNECTED) {
            tcpd.state = TCPD_CONNECTED;
        }
        break;

    case CYW43_LINK_FAIL:
        if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
            snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
            tcpd_log(s);
        } else {
            tcpd_log("WIFI RECONNECTING");
            tcpd.listening = false;
            tcpd.closed = false;
            tcpd.state = TCPD_CONNECTING;
        }
        break;

    case CYW43_LINK_NONET:
        tcpd.listening = false;
        tcpd.closed = false;
        break;

    case CYW43_LINK_BADAUTH:
        tcpd.listening = false;
        tcpd.closed = false;
        tcpd.state = TCPD_FATAL;
        break;

    case CYW43_LINK_NOIP:
        break;

    case CYW43_LINK_UP:
        if (!tcpd.listening) {
            tcpd_listen();
        } else {
            tcpd_server_monitor(&tcpd, tcpd.server);
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

    tcpd.server = tcp_listen_with_backlog(pcb, 1);
    if (!tcpd.server) {
        snprintf(s, sizeof(s), "LISTEN ERROR (%d)", err);
        tcpd_log(s);
        tcp_close(pcb);
        return;
    }

    tcp_arg(tcpd.server, &tcpd);
    tcp_accept(tcpd.server, tcpd_accept);

    tcpd_log("LISTENING");
    tcpd.listening = true;
    tcpd.closed = false;
    tcpd.idle = 0;
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

    for (int i = 0; i < CONNECTIONS; i++) {
        connection *conn = &connections[i];

        if (conn->client == NULL) {
            conn->server = tcpd.server;
            conn->client = client;
            conn->sent = 0;
            conn->received = 0;
            conn->idle = 0;

            tcp_arg(client, conn);
            tcp_sent(client, tcpd_sent);
            tcp_recv(client, tcpd_recv);
            tcp_poll(client, tcpd_client_monitor, TCP_CLIENT_POLL * 2);
            tcp_err(client, tcpd_err);

            return ERR_OK;
        }
    }

    // ... too many connections
    snprintf(s, sizeof(s), "%s:%d  TOO MANY CONNECTIONS", ip4addr_ntoa(&client->remote_ip), client->remote_port);
    tcpd_log(s);

    tcp_abort(client);

    return ERR_ABRT;
}

err_t tcpd_close() {
    char s[64];

    if (tcpd.server != NULL) {
        snprintf(s, sizeof(s), "%s:%d  CLOSING", ip4addr_ntoa(&tcpd.server->local_ip), tcpd.server->local_port);
        tcpd_log(s);
    } else {
        tcpd_log("CLOSING");
    }

    err_t err = ERR_OK;

    for (int i = 0; i < CONNECTIONS; i++) {
        connection *conn = &connections[i];
        struct tcp_pcb *p = conn->client;

        if (p != NULL) {
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
        }

        conn->server = NULL;
        conn->client = NULL;
    }

    if (tcpd.server) {
        tcp_arg(tcpd.server, NULL);
        tcp_close(tcpd.server);
        tcpd.server = NULL;
        tcpd.closed = true;
        tcpd.listening = false;
        tcpd.state = TCPD_UNKNOWN;
    }

    tcpd_log("CLOSED");

    return err;
}

err_t tcpd_recv(void *context, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    char s[64];

    connection *conn = (connection *)context;
    conn->idle = 0;

    // ... closed ?
    if (p == NULL) {
        snprintf(s, sizeof(s), "%s:%d  CLOSED", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_log(s);

        tcp_arg(pcb, NULL);
        tcp_poll(pcb, NULL, 0);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);

        conn->client = NULL;

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

            execw(cmd, reply, context);

            return ERR_OK;
        }
    }

    return ERR_OK;
}

err_t tcpd_send(void *context, struct tcp_pcb *pcb, const char *msg) {
    int N = strlen(msg);
    int ix = 0;
    int i = 0;

    connection *conn = (connection *)context;
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
err_t tcpd_sent(void *context, struct tcp_pcb *pcb, u16_t len) {
    char s[64];
    snprintf(s, sizeof(s), "%s:%d  SENT %d BYTES", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, len);
    tcpd_log(s);

    connection *conn = (connection *)context;
    conn->idle = 0;

    conn->sent += len;

    if (conn->sent >= TCP_BUFFER_SIZE) {
    }

    return ERR_OK;
}

err_t tcpd_server_monitor(void *context, struct tcp_pcb *pcb) {
    static int count = 0;

    char s[64];
    TCPD *tcpd = (TCPD *)context;
    const int N = TCP_SERVER_POLL * 1000 / TCPD_POLL;

    if (++count >= N) {
        count = 0;

        int clients = 0;
        for (int i = 0; i < CONNECTIONS; i++) {
            if (connections[i].client != NULL) {
                clients++;
            }
        }

        if (clients == 0) {
            tcpd->idle++;
        } else {
            tcpd->idle = 0;
        }

        if (tcpd->closed) {
            tcpd_log("CLOSED");
        } else if (clients == 0) {
            snprintf(s, sizeof(s), "%s:%d  SERVER IDLE %lus",
                     ip4addr_ntoa(&pcb->local_ip),
                     pcb->local_port,
                     tcpd->idle * TCP_SERVER_POLL);

            tcpd_log(s);
        } else {
            snprintf(s, sizeof(s), "%s:%d  CONNECTIONS:%d", ip4addr_ntoa(&pcb->local_ip), pcb->local_port, clients);
            tcpd_log(s);
        }

        if (tcpd->idle >= SERVER_IDLE_COUNT && !tcpd->closed) {
            tcpd_log("SHUTDOWN");
            tcpd_close();
        }
    }
}

err_t tcpd_client_monitor(void *context, struct tcp_pcb *pcb) {
    char s[64];
    err_t err = ERR_OK;

    connection *conn = (connection *)context;

    conn->idle++;

    if (conn->idle >= CLIENT_IDLE_COUNT) {
        snprintf(s, sizeof(s), "%s:%d  IDLE", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_log(s);

        tcp_arg(pcb, NULL);
        tcp_poll(pcb, NULL, 0);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);

        if ((err = tcp_close(pcb)) != ERR_OK) {
            snprintf(s, sizeof(s), "%s:%d  CLOSE ERROR (%d)", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, err);
            tcpd_log(s);
            tcp_abort(pcb);
            err = ERR_ABRT;
        }

        conn->client = NULL;

    } else {
        snprintf(s, sizeof(s), "%s:%d  OK", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_log(s);
    }

    return err;
}

void tcpd_err(void *context, err_t err) {
    char s[64];

    if (err != ERR_ABRT) {
        snprintf(s, sizeof(s), "ERROR %d", err);
        tcpd_log(s);
    }
}

void tcpd_log(const char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%-6s %s", "TCPD", msg);
    tx(s);
}
