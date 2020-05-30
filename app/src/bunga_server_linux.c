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

#include <dbg.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "bunga_server.h"

static void on_client_connected(struct bunga_server_t *self_p,
                                struct bunga_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    printf("Bunga client connected.\n");
}

static void on_client_disconnected(struct bunga_server_t *self_p,
                                   struct bunga_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    printf("Bunga client disconnected.\n");
}

static void on_execute_command_req(struct bunga_server_t *self_p,
                                   struct bunga_server_client_t *client_p,
                                   struct bunga_execute_command_req_t *request_p)
{
    (void)client_p;

    struct bunga_execute_command_rsp_t *response_p;

    response_p = bunga_server_init_execute_command_rsp(self_p);
    response_p->kind = bunga_execute_command_rsp_kind_output_e;
    response_p->output_p = "The output of '";
    bunga_server_reply(self_p);

    response_p = bunga_server_init_execute_command_rsp(self_p);
    response_p->kind = bunga_execute_command_rsp_kind_output_e;
    response_p->output_p = request_p->command_p;
    bunga_server_reply(self_p);

    response_p = bunga_server_init_execute_command_rsp(self_p);
    response_p->kind = bunga_execute_command_rsp_kind_ok_e;
    response_p->output_p = "' should go here.\n";
    bunga_server_reply(self_p);
}

static void *main()
{
    struct bunga_server_t server;
    struct bunga_server_client_t clients[2];
    uint8_t clients_input_buffers[2][128];
    uint8_t message[128];
    uint8_t workspace_in[128];
    uint8_t workspace_out[128];
    int epoll_fd;
    struct epoll_event event;
    int res;

    printf("Starting a Bunga server on ':28000'.\n");

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (NULL);
    }

    res = bunga_server_init(&server,
                            "tcp://:28000",
                            &clients[0],
                            2,
                            &clients_input_buffers[0][0],
                            sizeof(clients_input_buffers[0]),
                            &message[0],
                            sizeof(message),
                            &workspace_in[0],
                            sizeof(workspace_in),
                            &workspace_out[0],
                            sizeof(workspace_out),
                            on_client_connected,
                            on_client_disconnected,
                            on_execute_command_req,
                            epoll_fd,
                            NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (NULL);
    }

    res = bunga_server_start(&server);

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

        bunga_server_process(&server, event.data.fd, event.events);
    }

    return (NULL);
}

void bunga_server_linux_create(void)
{
    pthread_t pthread;

    pthread_create(&pthread, NULL, main, NULL);
}
