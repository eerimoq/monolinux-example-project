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
import systest

LOGGER = logging.getLogger(__name__)


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

    def stop(self):
        # ToDo.
        pass

    async def main(self):
        broker = mqttools.Broker('127.0.0.1', 1883)
        await broker.serve_forever()


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


class TestCase(systest.TestCase):

    def __init__(self, device):
        super().__init__()
        self.device = device
        self.mqtt_broker = None

    def send(self, line):
        self.device.sendline(line)

    def expect(self, text):
        for line in text.splitlines():
            if line:
                self.device.expect_exact(line)

    def expect_exact(self, text):
        self.device.expect_exact(text)

    def start_mqtt_broker(self):
        self.mqtt_broker = MqttBroker()
        self.mqtt_broker.start()

    def stop_mqtt_broker(self):
        if self.mqtt_broker is not None:
            self.mqtt_broker.stop()
            self.mqtt_broker = None


class StartupTest(TestCase):
    """Check startup output and that the shell is responsive.

    """

    def run(self):
        self.expect(
            "================= insmod begin =================\n"
            "Successfully inserted '/root/jbd2.ko'.\n"
            "Successfully inserted '/root/mbcache.ko'.\n"
            "Successfully inserted '/root/ext4.ko'.\n"
            "Loaded modules:\n"
            "=================== insmod end =================\n"
            "\n"
            "Welcome to Monolinux!\n"
            "\n"
            "Uptime:")
        self.send('')
        self.expect_exact('\n$ ')


class DiskTest(TestCase):
    """Test disk operations.

    """

    def run(self):
        self.send('test_disk')
        self.expect(
            "================ disk test begin ===============\n"
            "MOUNTED ON               TOTAL      USED      FREE\n"
            "/                        53 MB      2 MB     51 MB\n"
            "/proc                     0 MB      0 MB      0 MB\n"
            "/sys                      0 MB      0 MB      0 MB\n"
            "/mnt/disk1                7 MB      0 MB      7 MB\n"
            "/mnt/disk2                0 MB      0 MB      0 MB\n"
            "+---------------------------------------------------+\n"
            "|   __  __                   _ _                    |\n"
            "|  |  \/  |                 | (_)                   |\n"
            "|  | \  / | ___  _ __   ___ | |_ _ __  _   ___  __  |\n"
            "|  | |\/| |/ _ \| '_ \ / _ \| | | '_ \| | | \ \/ /  |\n"
            "|  | |  | | (_) | | | | (_) | | | | | | |_| |>  <   |\n"
            "|  |_|  |_|\___/|_| |_|\___/|_|_|_| |_|\__,_/_/\_\  |\n"
            "|                                                   |\n"
            "+---------------------------------------------------+\n"
            "================= disk test end ================\n")


class HeatshrinkTest(TestCase):
    """Test the Heatshrink library.

    """

    def run(self):
        self.send('test_heatshrink')
        self.expect(
            "============= heatshrink test begin ============\n"
            "Heatshrink encode and decode.\n"
            "============= heatshrink test end ==============\n")


class LzmaTest(TestCase):
    """Test the LZMA library.

    """

    def run(self):
        self.send('test_lzma')
        self.expect(
            "================ lzma test begin ===============\n"
            "LZMA decoder init successful.\n"
            "================= lzma test end ================\n")


class DetoolsTest(TestCase):
    """Test the detools library.

    """

    def run(self):
        self.send('test_detools')
        self.expect(
            "============== detools test begin ==============\n"
            "Library version:\n"
            "From:  /mnt/disk1/detools/v1.txt\n"
            "Patch: /mnt/disk1/detools/v1-v2.patch\n"
            "To:    /mnt/disk1/detools/v2.txt\n"
            "Before patching:\n"
            "+-----------+\n"
            "| Version 1 |\n"
            "+-----------+\n"
            "After patching:\n"
            "+-----------+\n"
            "| Version 2 |\n"
            "+-----------+\n"
            "detools: OK!\n"
            "=============== detools test end ===============\n")


class RtcTest(TestCase):
    """Test the RTC.

    """

    def run(self):
        self.send('test_rtc')
        self.expect(
            "================ RTC test begin ================\n"
            "RTC sysfs date:\n"
            "RTC sysfs time:\n"
            "RTC date and time:\n"
            "RTC date and time:\n"
            "================= RTC test end =================\n")


class HttpTest(TestCase):
    """Test HTTP.

    """

    def run(self):
        self.send('test_http')
        self.expect(
            "================ http test begin ===============\n"
            ">>> HTTP GET http://10.0.2.2:8001/\n"
            "<<< HTTP GET response code 200. <<<\n"
            ">>> HTTP GET https://10.0.2.2:4443/. >>>\n"
            "<<< HTTP GET CURL error code 7: Couldn't connect to server. <<<\n"
            "================= http test end ================\n")


class HttpsGetTest(TestCase):
    """Test HTTPS get.

    """

    def run(self):
        self.send('http_get https://example.com')
        self.expect(
            ">>> HTTP GET https://example.com. >>>\n"
            "<!doctype html>\n"
            "<<< HTTP GET response code 200. <<<\n")


class NtpClientTest(TestCase):
    """Test the NTP client.

    """

    def run(self):
        self.send('test_ntp_client')
        self.expect(
            "================ ntp client test begin ===============\n"
            "NTP client ok!\n"
            "================= ntp client test end ================\n")


class LogObjectTest(TestCase):
    """Test the log object module.

    """

    def run(self):
        self.send('test_log_object')
        self.expect(
            "============= log object test begin ============\n"
            "============== log object test end =============\n")
        self.send('dmesg')
        self.expect(
            "EMERGENCY foo Emergency level!\n"
            "INFO foo Info level!\n"
            "DEBUG foo Debug level!\n")


class NetworkFilterTest(TestCase):
    """Test the network filter module.

    """

    def run(self):
        self.send('test_network_filter')
        self.expect(
            "============= network filter test begin ============\n"
            ">>> HTTP GET http://example.com. >>>\n"
            "<<< HTTP GET response code 200. <<<\n"
            "Dropping HTTP.\n"
            ">>> HTTP GET http://example.com. >>>\n"
            "<<< HTTP GET CURL error code 28: Timeout was reached. <<<\n"
            "Accepting HTTP.\n"
            ">>> HTTP GET http://example.com. >>>\n"
            "<<< HTTP GET response code 200. <<<\n"
            "============== network filter test end =============\n")


class TcpServerTest(TestCase):
    """Test a TCP server.

    """

    def tcp_connect(self):
        sock = socket.socket()
        sock.connect(('localhost', 15000))
        sock.close()

    def run(self):
        self.send('test_tcp_server')
        self.expect(
            "============= TCP server test begin ============\n"
            "Listening for clients on port 15000.\n")
        self.tcp_connect()
        self.expect(
            "Client accepted.\n"
            "============= TCP server test end ============\n")


class MqttClientTest(TestCase):
    """Test the MQTT client.

    """

    def run(self):
        self.start_mqtt_broker()
        self.expect(
            "tcp: Connected.\n"
            "tcp: Starting the publish timer with timeout 1000 ms.\n"
            "tcp: Subscribe with transaction id 1 completed.\n"
            "tcp: Subscribe with transaction id 2 completed.\n"
            "tcp: Subscribe with transaction id 3 completed.\n"
            "tcp: Publishing 'count: 0' on 'async/hello'.\n"
            "tcp: Publishing 'count: 1' on 'async/hello'.\n")
        self.stop_mqtt_broker()


def main():
    systest.configure_logging(console_log_level=logging.DEBUG)

    http_server = HttpServer()
    http_server.start()

    device = pexpect.spawn('make -s run',
                           logfile=Logger(),
                           encoding='latin-1')

    sequencer = systest.Sequencer("Monolinux Example Project")

    _, failed, _ = sequencer.run(
        StartupTest(device),
        DiskTest(device),
        HeatshrinkTest(device),
        LzmaTest(device),
        DetoolsTest(device),
        RtcTest(device),
        HttpTest(device),
        HttpsGetTest(device),
        NtpClientTest(device),
        LogObjectTest(device),
        NetworkFilterTest(device),
        TcpServerTest(device),
        MqttClientTest(device)
    )

    exit_qemu(device)
    sequencer.report()
    sys.exit(failed)


main()
