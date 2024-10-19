""" 
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
"""

import socket
import sys


network_timeout = 1 # in s
udpPort = 4211
msg = "MVP3000"

sockListen = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockListen.bind(("0.0.0.0", udpPort))
print(f"Listening on port {udpPort}...")

# Send the message to the broadcast addresses of all interfaces
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP) as sock:
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(network_timeout)
    print (f"Sending to broadcast addresses...")
    sock.sendto(msg.encode('utf-8'), ("255.255.255.255", udpPort))

    message = "MVP3000"
    while message == "MVP3000":

        # Receive data from client (data, addr)
        data, addr = sockListen.recvfrom(1024)
        message = data.decode('utf-8')

        if message == "MVP3000":
            print (f"Discovery request from {addr[0]}")

        else:
            print(f"{message} @ {addr[0]}")
