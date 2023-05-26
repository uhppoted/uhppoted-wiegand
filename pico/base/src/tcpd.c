#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#include <logd.h>
#include <wiegand.h>

#include "tcpd.h"

#define TCP_BUFFER_SIZE 512

const uint8_t CR = 13;
const uint8_t LF = 10;
const uint16_t CLI_PORT = TCPD_CLI_PORT;
const uint16_t LOGD_PORT = TCPD_LOG_PORT;
const uint32_t TCPD_POLL = TCPD_POLL_INTERVAL; // ms
const uint8_t TCP_SERVER_POLL = 30;            // seconds
const uint8_t TCP_CLIENT_POLL = 5;             // seconds
const int CLIENT_IDLE_TIME = TCPD_CLIENT_IDLE; // seconds
const int CLIENT_IDLE_COUNT = CLIENT_IDLE_TIME / TCP_CLIENT_POLL;
const int SERVER_IDLE_TIME = TCPD_SERVER_IDLE; // seconds
const int SERVER_IDLE_COUNT = SERVER_IDLE_TIME / TCP_SERVER_POLL;

enum TCPD_STATE {
    TCPD_UNKNOWN = 0,
    TCPD_INITIALISED = 1,
    TCPD_CONNECTING = 2,
    TCPD_CONNECTED = 3,
    TCPD_FATAL = 4,
};

/* State for a client connection.
 *
 */
typedef struct connection {
    const char *tag;
    struct tcp_pcb *server;
    struct tcp_pcb *client;
    tcpd_handler handler;
    int sent;
    int received;
    int idle;
    uint8_t buffer_sent[TCP_BUFFER_SIZE];
    uint8_t buffer_recv[TCP_BUFFER_SIZE];
} connection;

static connection clients[2] = {
    {.tag = "TCPD", .server = NULL, .client = NULL, .handler = NULL, .sent = 0, .received = 0, .idle = 0},
    {.tag = "TCPD", .server = NULL, .client = NULL, .handler = NULL, .sent = 0, .received = 0, .idle = 0},
};

static connection loggers[2] = {
    {.tag = "LOGD", .server = NULL, .client = NULL, .handler = NULL, .sent = 0, .received = 0, .idle = 0},
    {.tag = "LOGD", .server = NULL, .client = NULL, .handler = NULL, .sent = 0, .received = 0, .idle = 0},
};

/* Internal state for the TCP server.
 *
 */
typedef struct TCPD {
    const char *tag;
    int link;
    enum TCPD_STATE state;
    bool listening;
    bool closed;
    uint32_t count;
    uint32_t idle;
    struct tcp_pcb *server;
    tcpd_handler handler;
    connection *connections;
} TCPD;

static TCPD tcpd = {
    .tag = "TCPD",
    .link = -100000,
    .state = TCPD_UNKNOWN,
    .listening = false,
    .closed = false,
    .count = 0,
    .idle = 0,
    .handler = tcpd_cli,
    .connections = clients,
};

static TCPD logd = {
    .tag = "LOGD",
    .link = -100000,
    .state = TCPD_UNKNOWN,
    .listening = false,
    .closed = false,
    .count = 0,
    .idle = 0,
    .handler = NULL,
    .connections = loggers,
};

const int CONNECTIONS = 2; // sizeof(connections) / sizeof(connection);
const int LOGGERS = 2;     // sizeof(loggers) / sizeof(connection);

/** Function prototypes **/

void tcpd_listen(TCPD *, uint16_t);
err_t tcpd_close(TCPD *);
err_t tcpd_accept(void *, struct tcp_pcb *, err_t);
err_t tcpd_recv(void *, struct tcp_pcb *, struct pbuf *, err_t);
err_t tcpd_send(void *, struct tcp_pcb *, const char *);
err_t tcpd_sent(void *, struct tcp_pcb *, u16_t);
err_t tcpd_server_monitor(void *, struct tcp_pcb *);
err_t tcpd_client_monitor(void *, struct tcp_pcb *);
void tcpd_reply(void *, const char *);
void tcpd_err(void *, err_t);
void tcpd_infof(const char *, const char *);

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
        tcpd_infof("TCPD", s);
        return false;
    }

    cyw43_arch_enable_sta_mode();

    if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
        snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
        tcpd_infof("TCPD", s);
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
    tcpd_infof("TCPD", "WIFI TERMINATED");
}

void tcpd_cli(void *context) {
    connection *conn = (connection *)context;
    conn->idle = 0;

    // ... CRLF?
    for (int i = 0; i < conn->received; i++) {
        if (conn->buffer_recv[i] == CR || conn->buffer_recv[i] == LF) {
            char cmd[64];
            int N = i < 64 ? i + 1 : 64;
            snprintf(cmd, N, "%s", conn->buffer_recv);

            conn->received = 0;

            execw(cmd, tcpd_reply, context);
            return;
        }
    }
}

void tcpd_log(const char *msg) {
    err_t err;

    for (int i = 0; i < LOGGERS; i++) {
        connection *conn = &logd.connections[i];

        if (conn->client != NULL) {
            if ((err = tcpd_send(conn, conn->client, msg)) != ERR_OK) {
                char s[64];
                snprintf(s, sizeof(s), "LOG ERROR (%d)", err);
                tcpd_infof("TCPD", s);
            }
        }
    }
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
            tcpd_infof("TCPD", "WIFI LINK DOWN");
            break;

        case CYW43_LINK_JOIN:
            if (tcpd.state != TCPD_CONNECTED) {
                tcpd_infof("TCPD", "WIFI CONNECTED");
                break;

            case CYW43_LINK_FAIL:
                tcpd_infof("TCPD", "WIFI CONNECTION FAILED");
                break;

            case CYW43_LINK_NONET:
                tcpd_infof("TCPD", "WIFI SSID NOT FOUND");
                break;

            case CYW43_LINK_BADAUTH:
                tcpd_infof("TCPD", "WIFI NOT AUTHORISED");
                break;

            case CYW43_LINK_NOIP:
                tcpd_infof("TCPD", "WIFI NO IP");
                break;

            case CYW43_LINK_UP:
                tcpd_infof("TCPD", "WIFI READY");
                break;

            default:
                tcpd_infof("TCPD", "WIFI UNKNOWN STATE");
            }
        }
    }

    switch (link) {
    case CYW43_LINK_DOWN:
        if (tcpd.state != TCPD_CONNECTING) {
            if ((err = cyw43_arch_wifi_connect_async(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK)) != 0) {
                snprintf(s, sizeof(s), "WIFI CONNECT ERROR (%d)", err);
                tcpd_infof("TCPD", s);
            } else {
                tcpd_infof("TCPD", "WIFI RECONNECTING");
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
            tcpd_infof("TCPD", s);
        } else {
            tcpd_infof("TCPD", "WIFI RECONNECTING");
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
            tcpd_listen(&tcpd, CLI_PORT);
        } else {
            tcpd_server_monitor(&tcpd, tcpd.server);
        }

        if (!logd.listening) {
            tcpd_listen(&logd, LOGD_PORT);
        } else {
            tcpd_server_monitor(&logd, logd.server);
        }
        break;
    }
}

void tcpd_listen(TCPD *tcpd, uint16_t port) {
    char s[64];
    err_t err;

    snprintf(s, sizeof(s), "ADDRESS %s PORT %u", ip4addr_ntoa(netif_ip4_addr(netif_list)), port);
    tcpd_infof(tcpd->tag, s);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        tcpd_infof(tcpd->tag, "ERROR CREAING pcb");
        return;
    }

    if ((err = tcp_bind(pcb, NULL, port)) != 0) {
        snprintf(s, sizeof(s), "ERROR BINDING TO PORT %u (%d)", port, err);
        tcpd_infof(tcpd->tag, s);
        return;
    }

    tcpd->server = tcp_listen_with_backlog(pcb, 1);
    if (!tcpd->server) {
        snprintf(s, sizeof(s), "LISTEN ERROR (%d)", err);
        tcpd_infof(tcpd->tag, s);
        tcp_close(pcb);
        return;
    }

    tcp_arg(tcpd->server, tcpd);
    tcp_accept(tcpd->server, tcpd_accept);

    tcpd_infof(tcpd->tag, "LISTENING");
    tcpd->listening = true;
    tcpd->closed = false;
    tcpd->idle = 0;
}

err_t tcpd_accept(void *context, struct tcp_pcb *client, err_t err) {
    TCPD *tcpd = (TCPD *)context;
    char s[64];

    snprintf(s, sizeof(s), "%s:%d  INCOMING", ip4addr_ntoa(&client->remote_ip), client->remote_port);
    tcpd_infof(tcpd->tag, s);

    // ... accept error?
    if (err != ERR_OK || client == NULL) {
        snprintf(s, sizeof(s), "ACCEPT ERROR (%d)", err);
        tcpd_infof(tcpd->tag, s);

        if (client != NULL) {
            err_t err = tcp_close(client);
            if (err != ERR_OK) {
                snprintf(s, sizeof(s), "CLOSE ERROR (%d)", err);
                tcpd_infof(tcpd->tag, s);
                tcp_abort(client);
                return ERR_ABRT;
            }
        }

        return ERR_VAL;
    }

    // ... connected!
    snprintf(s, sizeof(s), "%s:%d  CONNECTED", ip4addr_ntoa(&client->remote_ip), client->remote_port);
    tcpd_infof(tcpd->tag, s);

    for (int i = 0; i < CONNECTIONS; i++) {
        connection *conn = &tcpd->connections[i];

        if (conn->client == NULL) {
            conn->server = tcpd->server;
            conn->client = client;
            conn->handler = tcpd->handler;
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
    tcpd_infof(tcpd->tag, s);

    tcp_abort(client);

    return ERR_ABRT;
}

err_t tcpd_close(TCPD *tcpd) {
    char s[64];

    if (tcpd->server != NULL) {
        snprintf(s, sizeof(s), "%s:%d  CLOSING", ip4addr_ntoa(&tcpd->server->local_ip), tcpd->server->local_port);
        tcpd_infof(tcpd->tag, s);
    } else {
        tcpd_infof(tcpd->tag, "CLOSING");
    }

    err_t err = ERR_OK;

    for (int i = 0; i < CONNECTIONS; i++) {
        connection *conn = &tcpd->connections[i];
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
                tcpd_infof(tcpd->tag, s);
                tcp_abort(p);
                err = ERR_ABRT;
            }
        }

        conn->server = NULL;
        conn->client = NULL;
    }

    if (tcpd->server) {
        tcp_arg(tcpd->server, NULL);
        tcp_close(tcpd->server);
        tcpd->server = NULL;
        tcpd->closed = true;
        tcpd->listening = false;
        tcpd->state = TCPD_UNKNOWN;
    }

    tcpd_infof(tcpd->tag, "CLOSED");

    return err;
}

err_t tcpd_recv(void *context, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    char s[64];

    connection *conn = (connection *)context;
    conn->idle = 0;

    // ... closed ?
    if (p == NULL) {
        snprintf(s, sizeof(s), "%s:%d  CLOSED", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_infof("TCPD", s);

        tcp_arg(pcb, NULL);
        tcp_poll(pcb, NULL, 0);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);

        conn->client = NULL;

        return ERR_OK;
    }

    cyw43_arch_lwip_begin();
    if (p->tot_len > 0) {
        snprintf(s, sizeof(s), "WIFI RECV L:%d N:%d err:%d", p->tot_len, conn->received, err);
        tcpd_infof("TCPD", s);

        const uint16_t remaining = TCP_BUFFER_SIZE - conn->received;
        conn->received += pbuf_copy_partial(p,
                                            conn->buffer_recv + conn->received,
                                            p->tot_len > remaining ? remaining : p->tot_len, 0);
        tcp_recved(pcb, p->tot_len);
    }
    cyw43_arch_lwip_end();

    pbuf_free(p);

    // ... overflow?
    if (conn->received == TCP_BUFFER_SIZE || conn->handler == NULL) {
        conn->received = 0;
    } else {
        conn->handler(conn);
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

    cyw43_arch_lwip_begin();
    err_t err = tcp_write(pcb, conn->buffer_sent, i, TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        char s[64];
        snprintf(s, sizeof(s), "SEND ERROR (%d)", err);
        tcpd_infof("TCPD", s);

        return err;
    }

    return ERR_OK;
}

err_t tcpd_sent(void *context, struct tcp_pcb *pcb, u16_t len) {
    char s[64];
    snprintf(s, sizeof(s), "%s:%d  SENT %d BYTES", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, len);
    tcpd_infof("TCPD", s);

    connection *conn = (connection *)context;
    conn->idle = 0;

    conn->sent += len;

    if (conn->sent >= TCP_BUFFER_SIZE) {
    }

    return ERR_OK;
}

err_t tcpd_server_monitor(void *context, struct tcp_pcb *pcb) {
    TCPD *tcpd = (TCPD *)context;
    const uint32_t N = TCP_SERVER_POLL * 1000 / TCPD_POLL;
    char s[64];

    tcpd->count++;

    if (tcpd->count >= N) {
        tcpd->count = 0;

        int connected = 0;
        for (int i = 0; i < CONNECTIONS; i++) {
            if (tcpd->connections[i].client != NULL) {
                connected++;
            }
        }

        if (connected == 0) {
            tcpd->idle++;
        } else {
            tcpd->idle = 0;
        }

        if (tcpd->closed) {
            tcpd_infof(tcpd->tag, "CLOSED");
        } else if (connected == 0) {
            snprintf(s, sizeof(s), "%s:%d  SERVER IDLE %lus",
                     ip4addr_ntoa(&pcb->local_ip),
                     pcb->local_port,
                     tcpd->idle * TCP_SERVER_POLL);

            tcpd_infof(tcpd->tag, s);
        } else {
            snprintf(s, sizeof(s), "%s:%d  CONNECTIONS:%d", ip4addr_ntoa(&pcb->local_ip), pcb->local_port, connected);
            tcpd_infof(tcpd->tag, s);
        }

        if (tcpd->idle >= SERVER_IDLE_COUNT && !tcpd->closed) {
            tcpd_infof(tcpd->tag, "SHUTDOWN");
            tcpd_close(tcpd);
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
        tcpd_infof(conn->tag, s);

        tcp_arg(pcb, NULL);
        tcp_poll(pcb, NULL, 0);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);

        if ((err = tcp_close(pcb)) != ERR_OK) {
            snprintf(s, sizeof(s), "%s:%d  CLOSE ERROR (%d)", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, err);
            tcpd_infof(conn->tag, s);
            tcp_abort(pcb);
            err = ERR_ABRT;
        }

        conn->client = NULL;

    } else {
        snprintf(s, sizeof(s), "%s:%d  OK", ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);
        tcpd_infof(conn->tag, s);
    }

    return err;
}

void tcpd_reply(void *context, const char *msg) {
    char s[100];
    err_t err;

    connection *conn = (connection *)context;
    conn->idle = 0;

    snprintf(s, sizeof(s), ">  %s\r\n", msg);

    if ((err = tcpd_send(context, conn->client, s)) != ERR_OK) {
        snprintf(s, sizeof(s), "WIFI SEND ERROR (%d)", err);
        tcpd_infof("TCPD", s);
    }
}

void tcpd_err(void *context, err_t err) {
    char s[64];

    connection *conn = (connection *)context;

    if (err != ERR_ABRT) {
        snprintf(s, sizeof(s), "ERROR %d", err);
        tcpd_infof(conn->tag, s);
    }
}

void tcpd_infof(const char *tag, const char *msg) {
    char s[64];

    snprintf(s, sizeof(s), "%-6s %s", tag, msg);
    logd_log(s);
}
