# MVP3000 ESP32/ESP8266

Welcome to the **MVP3000 ESP32/ESP8266** repository, an integral part of the **MVP3000** rapid prototyping framework.

The MVP3000 framework is written for and by very early stage startup projects/companies. It aims to streamline the development and testing of new sensor or manipulation hardware and the consecutive transfer from the basic concept to a minimum viable product (MVP), a demonstrator device, or an independent setup for applications like manufacturing process control. It is build around readily available low-cost hardware and open software solutions.

A key feature of the framework is the seamless transition between the different development stages. Starting with the initial proof-of-concept script on the engineers laptop with a serial connection to perform first data analysis, moving to a laptop-independent data collection via wireless network allowing longer-term studies, towards a control device using a handheld Raspberry Pi with touchscreen.

1. Get the hardware running in a basic proof-of-concept script.
2. Include the MVP3000 framework in your code and feed it with the sensor data.
3. Use the web interface for main settings regarding data handling.
4. Receive data and send commands, all at the same time:  
    4.1. During development via the serial console,  
    4.2. Gain independence from wiring using WebSocket,  
    4.3. Record long-term data using an MQTT broker.  
5.  Use WebSocket or MQTT on an RaspberryPi to become independent from your laptop  build an MVP

More code will be shared in the future, mainly regarding data evaluation and setting up an MVP on a RaspberryPi.


## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [Getting Started](#GettingStarted)
	* [Installation](#Installation)
	* [First steps](#Firststeps)
	* [Web Interface](#WebInterface)
		* [Network](#Network)
		* [UDP Discovery](#UDPDiscovery)
		* [MQTT Communication](#MQTTCommunication)
	* [Code and Options](#CodeandOptions)
	* [Modules](#Modules)
* [Contribute and Create Custom Modules](#ContributeandCreateCustomModules)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->

## <a name='GettingStarted'></a>Getting Started

### <a name='Installation'></a>Installation

To install MVP3000 as Arduino library please clone or [download](https://github.com/Production3000/mvp3000esp/archive/refs/heads/main.zip) the main branch of this repository and follow the instructions for either [manual installation](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/#manual-installation), using the [Arduino IDE](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/#importing-a-zip-library), or in [PlatformIO](https://docs.platformio.org/en/latest/librarymanager/dependencies.html).
 
A listing in the library managers of PlatformIO and Arduino is targeted for a future release.

### <a name='Firststeps'></a>First steps

A good starting point is the [basic example](/examples/basic/basic.ino). It does not include any module, and thus is not very useful beyond testing. Additional functionality is provided for example by the [Sensor Module](/doc/sensor_module.md). It expects data which it then handles to provide additional functionality. The sensor read-out is part of the custom user-code and as such independent of the framework.

Compile a copy of the [basic example](/examples/basic/basic.ino) sketch and listen to the serial port using your IDE (baud rate: 115200). The ESP opens an access point. Connect to it with your computer and open its IP with your web browser to access the web interface.

The serial output will look like this, with a time stamp, the log category, and the actual message:

    0d 00:00:00 [I] Logger initialized.
    0d 00:00:00 [I] Led timing changed to: 0 ms
    0d 00:00:00 [W] File not found for reading: /cfgNet.json
    0d 00:00:00 [I] AP started: device13061407, 192.168.4.1
    0d 00:00:00 [W] File not found for reading: /cfgNetCom.json
    0d 00:00:00 [W] File not found for reading: /cfgNetMqtt.json
    0d 00:00:00 [I] Led timing changed to: 2000 ms
    0d 00:00:02 [U] This text will be printed in purple to Serial and the log-websocket.
    ...

Use the [WebSocket log example](/examples/websocket/websocket_log.html) to view the log output in a browser.

The LED indicates the device status:

 *  On - Connected as client to WiFi network.
 *  Slow blinking - Openen an access point without password to connect to for setup.
 *  Blinking - Connecting to WiFi network using the given credentials.
 *  Fast blinking - Fatal error, MVP3000 not operational. This should never happen!


### <a name='WebInterface'></a>Web Interface

The main page prints basic system information and the most recent log entries. It links to the configuration options, as described in the following, and also lists the loaded modules.

#### <a name='Network'></a>Network

 *  Enter the network credentials of your local network in order to connect the ESP.
 *  Connect tries on boot before credentials are deemed wrong and an AP is opened. If the network was seen since restart but the connection was lost, the ESP will try to reconnect forever until restart.
 *  Force client mode on boot. WARNING, if the credentials are wrong, the device will be inaccessible via the network and thus require re-flashing! 

#### <a name='UDPDiscovery'></a>UDP Discovery

 *  Enable/disable. This is a soft-disable, see udpHardDisable below.
 *  Port to use. This needs to be in accordance with the server-side pendant.
 *  Discovered server, other ESP devices are not printed

#### <a name='MQTTCommunication'></a>MQTT Communication

 *  Enable/disable.
 *  Connection status.
 *  The external broker overrides any discoverd local broker.
 *  MQTT port.
 *  List of active (_data) and subscribed (_ctrl) topics.


### <a name='CodeandOptions'></a>Code and Options

Include the framework into your script:

    #include <MVP3000.h>
    extern MVP3000 mvp;

    void setup() {
        // Your custom code
        mvp.setup();
    }

    void loop() {
        // Your custom code
        mvp.loop();
    }

The logger can be used to send message to serial and the websocket.

    mvp.log("This text will be printed in purple to Serial and the log-websocket.");

The serial output is color-coded using ANSI escape sequences. If your serial monitor does not support this feature (Arduino IDE) it can be turned off to omit the symbols. 

    mvp.logAnsiColor(false);

The framework uses UDP auto-discovery to find local servers. In case this interferes with your code it can be completely turned off.

    mvp.udpHardDisable();

### <a name='Modules'></a>Modules

Different modules can be loaded into the framework. First steps, examples, and options are given in the documentation of the respective module.

 *  [Sensor Module](/doc/sensor_module.md): The sensor module receives sensor data from the user script and processes it. It handles averaging, offset, scaling, and Tare. It publishes the live data to serial, to WebSocket, and MQTT. Stored recent data can be downloaded as CSV.
 *  LED Module (planned)
 *  Manipulator Module (planned)


## <a name='ContributeandCreateCustomModules'></a>Contribute and Create Custom Modules

Most user will not need to create a custom module. In order to develop and integrate a new module into the MVP 3000 framework please follow the documentation to [Custom Modules](/doc/custom_modules.md). 

Please follow the [GitHub guide](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project) on how to contribute to a project.


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
