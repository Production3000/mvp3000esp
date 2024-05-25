# MVP3000 ESP32/ESP8266

Welcome to the **MVP3000 ESP32/ESP8266** repository, an integral part of the **MVP3000** rapid prototyping framework.

The **MVP3000** framework is written for and by very early stage startup projects/companies. It aims to streamline the development and testing of new sensor or manipulation hardware and the consecutive transfer from the basic concept to a minimum viable product (MVP), a demonstrator device, or an independent setup for applications like manufacturing process control. It is build around readily available low-cost hardware and open software solutions.

A key feature of the framework is the seamless transition between the different development stages. Starting with the initial proof-of-concept script on the engineers laptop with a serial connection to perform first data analysis, moving to a laptop-independent data collection via wireless network allowing longer-term studies, towards a controll device using a handheld Raspberry Pi with touchscreen.

The framework comprises three main repositories:
1.  **MVP3000 ESP32/ESP8266** written in C++. It offers a web interface for main settings including data processing such as averaging, offset, and scaling, as well as data transfer options via serial interface or wireless to a MQTT broker.
2.  **[MVP3000 Controller](https://github.com/Production3000/mvp3000controller)**. Designed to run on a Raspberry Pi or a dedicated computer, this repository supports long-term data collection and demonstration capabilities independent of an engineer's laptop. It acts as MQTT broker and stores the received data in a MariaDB table. It includes a web kiosk interface for modern, touch-based user interactions.
3.  **[MVP3000 Evaluation](https://github.com/Production3000/mvp3000evaluation)** written in Python. The class receives data either via the serial interface or from the MQTT broker and allows basic processing such as averaging, offset, and scaling on the client side. It includes a number of scripts to display the data, record single or repeated or continuous measurements, and to perform basic standard analysis.



## Getting Started

### Installation

Install as Arduino library.

### Modules

First steps, examples, and detailed options are given in the documentation of the respective module

 *  [Sensor Module](/doc/sensor_module.md): The sensor module recieves sensor data from a user script and processes it. It is then output in CSV format via serial console and optionally published to a MQTT broker.
 *  [LED Module](): planned.


## Create Custom Modules

In order to develop and integrate a new module into the MVP 3000 framwork please follow the documentation to [Custom Modules](/doc/custom_modules.md).

Please request a pull or let us know about the great achievements you achieved so we can add a link.


## License

Licensed under the Apache License. See `LICENSE` for more information.

Copyright Production 3000
>>>>>>> dev
