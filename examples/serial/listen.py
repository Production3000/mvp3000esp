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

import serial

port = 'COM0' # Windows
# port = '/dev/ttyUSB0' # ESP8266
# port = '/dev/ttyACM0' # ESP32

ser = serial.Serial(port, 115200, timeout=10)
print(f"Listening on {ser.name} ...")

try:
    while True:
        line = ser.readline().decode('utf-8').strip()
        print(line)

except:
    # Catch Ctrl-C break
    True

ser.close()