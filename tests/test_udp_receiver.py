import sys
import socket
import threading
import time
import unittest

sys.path.insert(0, r"C:\Users\gapcu\Documents\vscode\SDKApp\ETS2sdkapp\python")

from udp_receiver import UDPReceiver

class UDPReceiverTests(unittest.TestCase):
    def test_receive_local_packet(self):
        received = []
        errors = []
        receiver = UDPReceiver("127.0.0.1", 12346, received.append, errors.append)
        receiver.start()
        time.sleep(0.1)

        sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sender.sendto(b'{"test":true}', ("127.0.0.1", 12346))
        sender.close()

        deadline = time.time() + 1.0
        while not received and time.time() < deadline:
            time.sleep(0.02)

        receiver.stop()
        time.sleep(0.05)
        self.assertEqual(received, [b'{"test":true}'])
        self.assertEqual(errors, [])

if __name__ == "__main__":
    unittest.main()
