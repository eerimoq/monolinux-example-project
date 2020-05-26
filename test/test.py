#!/usr/bin/env python3

import socket
import logging
import asyncio
import threading
import sys
import time
from http.server import SimpleHTTPRequestHandler
import socketserver
import pexpect
import mqttools


LOGGER = logging.getLogger(__name__)

STARTUP_OUTPUT = '''\
================= insmod begin =================
Successfully inserted '/root/jbd2.ko'.
Successfully inserted '/root/mbcache.ko'.
Successfully inserted '/root/ext4.ko'.
Loaded modules:
=================== insmod end =================

Welcome to Monolinux!

Uptime:

================ disk test begin ===============
MOUNTED ON               TOTAL      USED      FREE
/                        53 MB      2 MB     51 MB
/proc                     0 MB      0 MB      0 MB
/sys                      0 MB      0 MB      0 MB
/mnt/disk1                7 MB      0 MB      7 MB
/mnt/disk2                0 MB      0 MB      0 MB
+---------------------------------------------------+
|   __  __                   _ _                    |
|  |  \/  |                 | (_)                   |
|  | \  / | ___  _ __   ___ | |_ _ __  _   ___  __  |
|  | |\/| |/ _ \| '_ \ / _ \| | | '_ \| | | \ \/ /  |
|  | |  | | (_) | | | | (_) | | | | | | |_| |>  <   |
|  |_|  |_|\___/|_| |_|\___/|_|_|_| |_|\__,_/_/\_\  |
|                                                   |
+---------------------------------------------------+
================= disk test end ================

============= heatshrink test begin ============
Heatshrink encode and decode.
============= heatshrink test end ==============

================ lzma test begin ===============
LZMA decoder init successful.
================= lzma test end ================

============== detools test begin ==============
Library version:
From:  /mnt/disk1/detools/v1.txt
Patch: /mnt/disk1/detools/v1-v2.patch
To:    /mnt/disk1/detools/v2.txt
Before patching:
+-----------+
| Version 1 |
+-----------+
After patching:
+-----------+
| Version 2 |
+-----------+
detools: OK!
=============== detools test end ===============

================ RTC test begin ================
RTC sysfs date:
RTC sysfs time:
RTC date and time:
RTC date and time:
================= RTC test end =================

================ http test begin ===============
>>> HTTP GET http://10.0.2.2:8001/
<<< HTTP GET response code 200. <<<
>>> HTTP GET https://10.0.2.2:4443/. >>>
<<< HTTP GET CURL error code 7: Couldn't connect to server. <<<
================= http test end ================

================ ntp client test begin ===============
NTP client ok!
================= ntp client test end ================

============= log object test begin ============
============== log object test end =============

============= network filter test begin ============
============== network filter test end =============

============= TCP server test begin ============
Listening for clients on port 15000.
'''


def expect_text(child, text):
    for line in text.splitlines():
        if line:
            child.expect_exact(line)


class TCPServer(socketserver.TCPServer):

    allow_reuse_address = True


class HttpServer(threading.Thread):

    def __init__(self):
        super().__init__()
        self.daemon = True

    def run(self):
        with TCPServer(("", 8001), SimpleHTTPRequestHandler) as httpd:
            httpd.serve_forever()


class MqttBroker(threading.Thread):

    def __init__(self):
        super().__init__()
        self.daemon = True

    def run(self):
        asyncio.run(self.main())

    async def main(self):
        broker = mqttools.Broker('127.0.0.1', 1883)
        await broker.serve_forever()


TCP_CONNECT_OUTPUT = '''\
Client accepted.
============= TCP server test end ============
'''


def tcp_connect(child):
    sock = socket.socket()
    sock.connect(('localhost', 15000))
    sock.close()
    expect_text(child, TCP_CONNECT_OUTPUT)


HTTPS_OUTPUT = '''\
>>> HTTP GET https://example.com. >>>
<!doctype html>
<<< HTTP GET response code 200. <<<
'''


def https_get_example_com(child):
    child.sendline('http_get https://example.com')
    expect_text(child, HTTPS_OUTPUT)


MQTT_OUTPUT = '''\
tcp: Starting the MQTT client, connecting to '10.0.2.2:1883'.
ssl: Starting the MQTT client, connecting to '10.0.2.2:8883'.
tcp: Connected.
tcp: Starting the publish timer with timeout 1000 ms.
tcp: Subscribe with transaction id 1 completed.
tcp: Subscribe with transaction id 2 completed.
tcp: Subscribe with transaction id 3 completed.
tcp: Publishing 'count: 0' on 'async/hello'.
tcp: Publishing 'count: 1' on 'async/hello'.
'''

DMESG_TEXT = '''\
EMERGENCY foo Emergency level!
INFO foo Info level!
DEBUG foo Debug level!
INFO default Dropping HTTP for 10 seconds.
'''


def exit_qemu(child):
    child.sendcontrol('a')
    child.send('x')

    while child.isalive():
        time.sleep(0.1)


class Logger:

    def __init__(self):
        self._data = ''

    def write(self, data):
        line = ''

        for char in self._data + data:
            if char == '\n':
                LOGGER.info('app: %s', line.strip('\r\n\v'))
                line = ''
            else:
                line += char

        self._data = line

    def flush(self):
        pass


def main():
    logging.basicConfig(level=logging.DEBUG)

    http_server = HttpServer()
    http_server.start()
    mqtt_broker = MqttBroker()
    mqtt_broker.start()

    child = pexpect.spawn('make -s run',
                          logfile=Logger(),
                          encoding='latin-1')

    expect_text(child, STARTUP_OUTPUT)
    tcp_connect(child)
    expect_text(child, MQTT_OUTPUT)
    child.sendline('dmesg')
    expect_text(child, DMESG_TEXT)
    https_get_example_com(child)
    exit_qemu(child)

    print('Done!')


main()