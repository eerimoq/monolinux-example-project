/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019, Erik Moqvist
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <curl/curl.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>
#include <lzma.h>
#include <detools.h>
#include "async_main.h"
#include "chat_server_linux.h"
#include "bunga_server_linux.h"
#include "network_filter.h"
#include "ml/ml.h"

struct folder_t {
    const char *path_p;
    int mode;
};

extern int command_lzma_compress(int argc, const char *argv[], FILE *fout_p);

static void insert_modules(void)
{
    int res;
    int i;
    static const char *modules[] = {
        "/root/jbd2.ko",
        "/root/mbcache.ko",
        "/root/ext4.ko"
    };

    printf("================= insmod begin =================\n");

    for (i = 0; i < membersof(modules); i++) {
        res = ml_insert_module(modules[i], "");

        if (res == 0) {
            printf("Successfully inserted '%s'.\n", modules[i]);
        } else {
            printf("Failed to insert '%s'.\n", modules[i]);
        }
    }

    printf("Loaded modules:\n");
    ml_print_file("/proc/modules", stdout);

    printf("=================== insmod end =================\n\n");
}

static void create_folders(void)
{
    static const struct folder_t folders[] = {
        { "/proc", 0644 },
        { "/sys", 0444 },
        { "/dev/mapper", 0644 },
        { "/mnt", 0644 },
        { "/mnt/disk1", 0644 },
        { "/mnt/disk2", 0644 },
        { "/etc", 0644 }
    };
    int i;
    int res;

    for (i = 0; i < membersof(folders); i++) {
        res = mkdir(folders[i].path_p, folders[i].mode);

        if (res != 0) {
            ml_error("Failed to create '%s'", folders[i].path_p);
        }
    }
}

static void pmount(const char *source_p,
                   const char *target_p,
                   const char *type_p,
                   unsigned long flags,
                   const char *options_p)
{
    int res;

    res = mount(source_p, target_p, type_p, flags, options_p);

    if (res != 0) {
        printf("Mount of %s failed with %s.\n", target_p, strerror(-res));
    }
}

static void pmknod(const char *path_p, mode_t mode, dev_t dev)
{
    int res;

    res = mknod(path_p, mode, dev);

    if (res != 0) {
        printf("Mknod of %s failed with %s.\n", path_p, strerror(-res));
    }
}

static void create_files(void)
{
    FILE *file_p;

    pmount("none", "/proc", "proc", 0, NULL);
    pmount("none", "/sys", "sysfs", 0, NULL);

    pmknod("/dev/null", S_IFCHR | 0644, makedev(1, 3));
    pmknod("/dev/zero", S_IFCHR | 0644, makedev(1, 5));
    pmknod("/dev/urandom", S_IFCHR | 0644, makedev(1, 9));
    pmknod("/dev/kmsg", S_IFCHR | 0644, makedev(1, 11));
    pmknod("/dev/sda1", S_IFBLK | 0666, makedev(8, 1));
    pmknod("/dev/sdb1", S_IFBLK | 0666, makedev(8, 16));
    pmknod("/dev/sdc1", S_IFBLK | 0666, makedev(8, 32));
    pmknod("/dev/mapper/control", S_IFCHR | 0666, makedev(10, 236));

    ml_device_mapper_verity_create(
        "erik",
        "00000000-1111-2222-3333-444444444444",
        "/dev/sdb1",
        8192,
        "/dev/sdc1",
        0,
        "af4f26725d8ce706744b54d313ba47ab3be890b76c592ede8aca52779f4e93c9",
        "7891234871263971625789623497586239875698273465987234658792364598");

    pmount("/dev/sda1", "/mnt/disk1", "ext4", 0, NULL);
    pmount("/dev/mapper/00000000-1111-2222-3333-444444444444",
           "/mnt/disk2",
           "squashfs",
           MS_RDONLY,
           NULL);

    file_p = fopen("/etc/resolv.conf", "w");

    if (file_p != NULL) {
        fwrite("nameserver 8.8.4.4\n", 19, 1, file_p);
        fclose(file_p);
    }
}

static void http_get(const char *url_p, FILE *fout_p)
{
    CURL *curl_p;
    long response_code;
    int res;

    fprintf(fout_p, ">>> HTTP GET %s. >>>\n", url_p);

    curl_p = curl_easy_init();

    if (curl_p) {
        curl_easy_setopt(curl_p, CURLOPT_URL, url_p);
        curl_easy_setopt(curl_p, CURLOPT_WRITEDATA, fout_p);
        curl_easy_setopt(curl_p, CURLOPT_CONNECTTIMEOUT, 5);

        /* WARNING: Makes the connection insecure! */
        curl_easy_setopt(curl_p, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl_p, CURLOPT_SSL_VERIFYHOST, 0);

        res = curl_easy_perform(curl_p);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl_p, CURLINFO_RESPONSE_CODE, &response_code);
            fprintf(fout_p, "<<< HTTP GET response code %ld. <<<\n", response_code);
        } else {
            fprintf(fout_p, "<<< HTTP GET CURL error code %d: %s. <<<\n",
                   res,
                   curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl_p);
    }
}

static int command_http_get(int argc, const char *argv[], FILE *fout_p)
{
    if (argc != 2) {
        fprintf(fout_p, "http_get <url>\n");

        return (-1);
    }

    http_get(argv[1], fout_p);

    return (0);
}

static int command_test_disk(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    fprintf(fout_p, "================ disk test begin ===============\n");
    ml_print_file_systems_space_usage(fout_p);
    ml_print_file("/mnt/disk1/README", fout_p);
    fprintf(fout_p, "================= disk test end ================\n\n");

    return (0);
}

static int command_test_heatshrink(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    fprintf(fout_p,
            "============= heatshrink test begin ============\n"
            "Heatshrink encode and decode.\n");

    (void)heatshrink_encoder_alloc(8, 4);
    (void)heatshrink_decoder_alloc(512, 8, 4);

    fprintf(fout_p,
            "============= heatshrink test end ==============\n\n");

    return (0);
}

static int command_test_lzma(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    lzma_ret ret;
    lzma_stream stream;

    fprintf(fout_p, "================ lzma test begin ===============\n");

    memset(&stream, 0, sizeof(stream));

    ret = lzma_alone_decoder(&stream, UINT64_MAX);

    if (ret != LZMA_OK) {
        fprintf(fout_p, "LZMA decoder init failed.\n");
    } else {
        fprintf(fout_p, "LZMA decoder init successful.\n");
    }

    fprintf(fout_p, "================= lzma test end ================\n\n");

    return (0);
}

static int command_test_detools(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    int res;
    static const char from[] = "/mnt/disk1/detools/v1.txt";
    static const char patch[] = "/mnt/disk1/detools/v1-v2.patch";
    static const char to[] = "/mnt/disk1/detools/v2.txt";

    fprintf(fout_p, "============== detools test begin ==============\n");
    fprintf(fout_p, "Library version: %s\n", DETOOLS_VERSION);
    fprintf(fout_p, "From:  %s\n", from);
    fprintf(fout_p, "Patch: %s\n", patch);
    fprintf(fout_p, "To:    %s\n", to);

    res = detools_apply_patch_filenames(from, patch, to);

    if (res >= 0) {
        fprintf(fout_p, "Before patching:\n");
        ml_print_file(from, fout_p);
        fprintf(fout_p, "After patching:\n");
        ml_print_file(to, fout_p);
        fprintf(fout_p, "detools: OK!\n");
    } else {
        res *= -1;
        fprintf(fout_p, "error: detools: %s (error code %d)\n",
               detools_error_as_string(res),
               res);
    }

    fprintf(fout_p, "=============== detools test end ===============\n\n");

    return (0);
}

static int command_test_rtc(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    struct tm tm;

    fprintf(fout_p, "================ RTC test begin ================\n");
    fprintf(fout_p, "RTC sysfs date: ");
    ml_print_file("/sys/class/rtc/rtc0/date", fout_p);
    fprintf(fout_p, "RTC sysfs time: ");
    ml_print_file("/sys/class/rtc/rtc0/time", fout_p);
    pmknod("/dev/rtc0", S_IFCHR | 0666, makedev(254, 0));
    ml_rtc_get_time("/dev/rtc0", &tm);
    fprintf(fout_p, "RTC date and time: %s", asctime(&tm));
    tm.tm_year++;
    ml_rtc_set_time("/dev/rtc0", &tm);
    ml_rtc_get_time("/dev/rtc0", &tm);
    fprintf(fout_p, "RTC date and time: %s", asctime(&tm));
    fprintf(fout_p, "================= RTC test end =================\n\n");

    return (0);
}

static int command_test_http(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    fprintf(fout_p, "================ http test begin ===============\n");
    http_get("http://10.0.2.2:8001/", fout_p);
    http_get("https://10.0.2.2:4443/", fout_p);
    fprintf(fout_p, "================= http test end ================\n\n");

    return (0);
}

static int command_test_ntp_client(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    int res;

    fprintf(fout_p, "================ ntp client test begin ===============\n");
    res = ml_ntp_client_sync("0.se.pool.ntp.org");

    if (res == 0) {
        fprintf(fout_p, "NTP client ok!\n");
    } else {
        fprintf(fout_p, "NTP client failed!\n");
    }

    fprintf(fout_p, "================= ntp client test end ================\n\n");

    return (0);
}

static int command_test_log_object(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    struct ml_log_object_t log_object;

    fprintf(fout_p, "============= log object test begin ============\n");
    ml_log_object_init(&log_object, "foo", ML_LOG_DEBUG);
    ml_log_object_print(&log_object, ML_LOG_EMERGENCY, "Emergency level!");
    ml_log_object_print(&log_object, ML_LOG_INFO, "Info level!");
    ml_log_object_print(&log_object, ML_LOG_DEBUG, "Debug level!");
    fprintf(fout_p, "============== log object test end =============\n\n");

    return (0);
}

static int command_test_network_filter(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    fprintf(fout_p, "============= network filter test begin ============\n");
    http_get("http://example.com", fout_p);
    fprintf(fout_p, "Dropping HTTP.\n");
    network_filter_drop_http();
    http_get("http://example.com", fout_p);
    ml_network_filter_ipv4_accept_all();
    fprintf(fout_p, "Accepting HTTP.\n");
    http_get("http://example.com", fout_p);
    fprintf(fout_p, "============== network filter test end =============\n\n");

    return (0);
}

static int command_test_tcp_server(int argc, const char *argv[], FILE *fout_p)
{
    (void)argc;
    (void)argv;

    int listener;
    int client;
    struct sockaddr_in addr;
    int res;

    fprintf(fout_p, "============= TCP server test begin ============\n");

    listener = socket(AF_INET, SOCK_STREAM, 0);

    if (listener == -1) {
        perror("socket");
        goto out;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(15000);

    res = bind(listener, (struct sockaddr*)&addr, sizeof(addr));

    if (res == -1) {
        perror("bind");
        close(listener);
        goto out;
    }

    res = listen(listener, 10);

    if (res == -1) {
        perror("listen");
        close(listener);
        goto out;
    }

    fprintf(fout_p, "Listening for clients on port 15000.\n");

    client = accept(listener, (struct sockaddr*)NULL, NULL);

    if (client == -1) {
        perror("accept");
    } else {
        fprintf(fout_p, "Client accepted.\n");
        close(client);
    }

    close(listener);

 out:
    fprintf(fout_p, "============= TCP server test end ============\n");

    return (0);
}

static void init(void)
{
    insert_modules();
    create_folders();
    create_files();
    ml_init();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    ml_shell_init();
    ml_network_init();
    ml_shell_register_command("lzmac",
                              "LZMA compress.",
                              command_lzma_compress);
    ml_shell_register_command("http_get",
                              "HTTP GET.",
                              command_http_get);
    ml_shell_register_command("test_disk",
                              "Test disk operations.",
                              command_test_disk);
    ml_shell_register_command("test_heatshrink",
                              "Test the Heatshrink library.",
                              command_test_heatshrink);
    ml_shell_register_command("test_lzma",
                              "The the LZMA library.",
                              command_test_lzma);
    ml_shell_register_command("test_detools",
                              "Test the detools library.",
                              command_test_detools);
    ml_shell_register_command("test_rtc",
                              "Test the RTC.",
                              command_test_rtc);
    ml_shell_register_command("test_http",
                              "Test HTTP.",
                              command_test_http);
    ml_shell_register_command("test_ntp_client",
                              "Test NTP time synch.",
                              command_test_ntp_client);
    ml_shell_register_command("test_log_object",
                              "Test the log object module.",
                              command_test_log_object);
    ml_shell_register_command("test_network_filter",
                              "The the network filter module.",
                              command_test_network_filter);
    ml_shell_register_command("test_tcp_server",
                              "Test a TCP server.",
                              command_test_tcp_server);
    ml_shell_start();
}

static void print_banner(void)
{
    int fd;
    char buf[128];
    ssize_t size;

    fd = open("/proc/uptime", O_RDONLY);

    if (fd < 0) {
        perror("error: ");

        return;
    }

    size = read(fd, &buf[0], sizeof(buf) - 1);

    if (size >= 0) {
        buf[size] = '\0';
        strtok(&buf[0], " ");
    } else {
        strcpy(&buf[0], "-");
    }

    close(fd);

    printf("Welcome to Monolinux!\n"
           "\n"
           "Uptime: %s\n"
           "\n",
           &buf[0]);
}

int main(int argc, const char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "finalize_coredump") == 0) {
            ml_finalize_coredump("/mnt/disk1/coredumps", 3);
        }
    }

    init();

    print_banner();
    ml_network_interface_up("eth0");

# if 1
    ml_network_interface_configure("eth0",
                                   "10.0.2.15",
                                   "255.255.255.0",
                                   1500);
    ml_network_interface_add_route("eth0", "10.0.2.2");
#else
    struct ml_dhcp_client_t dhcp_client;

    ml_dhcp_client_init(&dhcp_client, "eth0", ML_LOG_INFO);
    ml_dhcp_client_start(&dhcp_client);
#endif

    chat_server_linux_create();
    bunga_server_linux_create();

    return (async_main());
}
