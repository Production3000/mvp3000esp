# MVP3000 ESP32/ESP8266

Welcome to the **MVP3000 ESP32/ESP8266** repository, an integral part of the **MVP3000** rapid prototyping framework

The **MVP3000** framework is written for and by very early stage startup projects/companies. It aims to streamline the development and testing of new sensor or manipulation hardware and the consecutive transfer from the basic concept to a minimum viable product (MVP), a demonstrator device, or an independent setup for applications like manufacturing process control. It is build around readily available low-cost hardware and open software solutions.

A key feature of the framework is the seamless transition between the different development stages. Starting with the initial proof-of-concept script on the engineers laptop with a serial connection to perform first data analysis, moving to a laptop-independent data collection via wireless network allowing longer-term studies, towards a controll device using a handheld Raspberry Pi with touchscreen.

The framework comprises three main repositories:
1.  **MVP3000 ESP32/ESP8266** written in C++ (in the porcess of being open-sourced). It offers a web interface for main settings including data processing such as averaging, offset, and scaling, as well as data transfer options via serial interface or wireless to a MQTT broker.
2.  **[MVP3000 Evaluation](https://github.com/Production3000/mvp3000evaluation)** written in Python. The class receives data either via the serial interface or from the MQTT broker and allows basic processing such as averaging, offset, and scaling on the client side. It includes a number of scripts to display the data, record single or repeated or continuous measurements, and to perform basic standard analysis.
3.  **[MVP3000 Controller](https://github.com/Production3000/mvp3000controller)** (to be open-sourced). Designed to run on a Raspberry Pi or a dedicated computer, this repository supports long-term data collection and demonstration capabilities independent of an engineer's laptop. It acts as MQTT broker and stores the recieved data in a MariaDB table. It includes a web kiosk interface for modern, touch-based user interactions.



## Quick Start

Please refer to the documentation for detailed options. (once it is written)

### Installation

Install as Arduino library.


### General Workflow

1.  Get a proof-of-concept script running
1.  Include the MVP3000 framework in your code
3.  Feed the sensor data into the framework
4.  Set averaging, offset, scaling either at compile-time in the code or on-the-fly via a web interface.
5.  View live data, measure noise, and gather data using the scripts provided in **[MVP3000 Evaluation](https://github.com/Production3000/mvp3000evaluation)**.
6.  Gain independence from your laptop using the **[MVP3000 Controller](https://github.com/Production3000/mvp3000controller)** to collect long-term data, build an independent setup for others to use, or a minimum viable product or demonstrator device.
7.  ???
8.  Profit


### Basic Sensor Example

A good starting point are the [examples](https://github.com/Production3000/mvp3000/tree/main/examples), particularly [basic.ino](https://github.com/Production3000/mvp3000/tree/main/examples/basic.ino).



## Troubleshooting

Q: My raw sensor values are good, but the averaged values reported by the framework are zero/strange?  
A: Check offset and scaling via the web interface. Possibly factory reset the device.



## License

Licensed under the Apache License. See `LICENSE` for more information.

Copyright Production 3000
