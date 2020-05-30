/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Monolinux project.
 */

#include <stdio.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "chat_server.h"

static int number_of_connected_clients = 0;

static void on_client_disconnected(struct chat_server_t *self_p,
                                   struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    number_of_connected_clients--;

    printf("chat-server: Number of connected clients: %d\n",
           number_of_connected_clients);
}

static void on_connect_req(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_connect_req_t *message_p)
{
    (void)client_p;

    number_of_connected_clients++;

    printf("chat-server: Client <%s> connected.\n", message_p->user_p);
    printf("chat-server: Number of connected clients: %d\n",
           number_of_connected_clients);

    chat_server_init_connect_rsp(self_p);
    chat_server_reply(self_p);
}

static void on_message_ind(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_message_ind_t *message_in_p)
{
    (void)client_p;

    struct chat_message_ind_t *message_p;

    message_p = chat_server_init_message_ind(self_p);
    message_p->user_p = message_in_p->user_p;
    message_p->text_p = message_in_p->text_p;
    chat_server_broadcast(self_p);
}

static void *main()
{
    struct chat_server_t server;
    struct chat_server_client_t clients[10];
    uint8_t clients_input_buffers[10][128];
    uint8_t message[128];
    uint8_t workspace_in[128];
    uint8_t workspace_out[128];
    int epoll_fd;
    struct epoll_event event;
    int res;
    const char *uri_p;

    printf("Starting a chat server on ':6000'.\n");

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (NULL);
    }

    res = chat_server_init(&server,
                           "tcp://:6000",
                           &clients[0],
                           10,
                           &clients_input_buffers[0][0],
                           sizeof(clients_input_buffers[0]),
                           &message[0],
                           sizeof(message),
                           &workspace_in[0],
                           sizeof(workspace_in),
                           &workspace_out[0],
                           sizeof(workspace_out),
                           NULL,
                           on_client_disconnected,
                           on_connect_req,
                           on_message_ind,
                           epoll_fd,
                           NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (NULL);
    }

    res = chat_server_start(&server);

    if (res != 0) {
        printf("Start failed.\n");

        return (NULL);
    }

    printf("Server started.\n");

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        chat_server_process(&server, event.data.fd, event.events);
    }

    return (NULL);
}

void chat_server_linux_create(void)
{
    pthread_t pthread;

    pthread_create(&pthread, NULL, main, NULL);
}
