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


udpPort = 4211

# Pretend to have a MQTT server running. The device will try to connect and then give up after a few tries.
# One can also add a second skill to the response for a custom module.
skills = "MQTT;SKILL2"
response = f"SERVER;{skills}"

# Create a new socket, define the port on which to listen
sockListen = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockListen.bind(("0.0.0.0", udpPort))
print(f"Listening on port {udpPort}...")

while True:
    data, addr = sockListen.recvfrom(1024)
    message = data.decode('utf-8')

    if message == "MVP3000":
        # Send the response to the original sender
        sockRespond = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sockRespond.sendto(response.encode('utf-8'), (addr[0], udpPort))
        # Close the response socket
        sockRespond.close()

        print(f"Discovery recieved from {addr[0]}.")

    else:
        print(f"UNKNOWN: {message}")
