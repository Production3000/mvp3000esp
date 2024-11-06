# MVP3000 ESP32/ESP8266

Welcome to the **MVP3000 ESP32/ESP8266** repository, an integral part of the [**MVP3000**](https://www.production3000.com/mvp3000/) rapid prototyping framework.

## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [Introduction](#Introduction)
* [Getting Started](#GettingStarted)
	* [Installation](#Installation)
	* [First Steps](#FirstSteps)
	* [LED Status Indication](#LEDStatusIndication)
	* [Web Interface](#WebInterface)
	* [WebSockets](#WebSockets)
	* [MQTT Communication](#MQTTCommunication)
	* [UDP Auto Discovery](#UDPAutoDiscovery)
* [Modules](#Modules)
* [Implementation](#Implementation)
	* [General Functionality](#GeneralFunctionality)
	* [No Blocking delay() on ESP](#NoBlockingdelayonESP)
	* [Public Methods and Options](#PublicMethodsandOptions)
	* [Helper Functions and Classes](#HelperFunctionsandClasses)
	* [Custom Modules](#CustomModules)
* [Application Note: CO2-Lights (CO2-Ampel)](#ApplicationNote:CO2-LightsCO2-Ampel)
* [Troubleshooting](#Troubleshooting)
* [Contribute](#Contribute)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->


## <a name='Introduction'></a>Introduction

The MVP3000 framework is built to streamline the implementation of sensor and actuator hardware. It facilitates a seamless transitions between all development stages and thus speeds up the implementation cycle. It allows the rapid development of a MVP (minimum viable product) for demonstration purposes. It allows the rapid deployment in fabrication for the purpose of deskilling and improving process reliability. Designed around low-cost hardware and open-source software, its modular structure promotes fast, flexible, and agile hardware development.

A key feature of the framework is the seamless transition between the different development stages. Starting with the initial proof-of-concept script on the engineers laptop with a serial connection to perform first data analysis, moving to a laptop-independent data collection via wireless network allowing longer-term studies, towards a control device using a handheld Raspberry Pi with touchscreen.

1. Get the hardware running in a basic proof-of-concept script.
2. Include the MVP3000 framework in your code and feed it with the sensor data.
3. Use the web interface for main settings regarding data handling.
4. Receive data and send commands, all at the same time:  
    4.1. During development via the serial console.  
    4.2. Gain independence from wiring using WebSocket.  
    4.3. Remote data recording using an MQTT broker.  
5.  Use WebSocket or MQTT on an RaspberryPi to become laptop-independent and build an MVP

## <a name='GettingStarted'></a>Getting Started

### <a name='Installation'></a>Installation

To install MVP3000 as Arduino library please clone or [download](https://github.com/Production3000/mvp3000esp/archive/refs/heads/main.zip) the main branch of this repository and follow the instructions for either [manual installation](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/#manual-installation), using the [Arduino IDE](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/#importing-a-zip-library), or in [PlatformIO](https://docs.platformio.org/en/latest/librarymanager/dependencies.html).
 
A listing in the library managers of PlatformIO and Arduino is targeted for a future release.

### <a name='FirstSteps'></a>First Steps

A good starting point is the [basic example](/examples/mvp/basic/basic.ino). It does not include any module, and thus is not very useful beyond testing.

Compile a copy of the sketch and listen to the serial port using your IDE (baud rate: 115200).
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

The ESP opens an access point. Connect to it with your computer and open its IP with your web browser to access the [web interface](#web-interface).

Use the [WebSocket log example](/examples/websocket/websocket_log.html) to view the log output in a browser.

As a next step proceed with using one of the [modules](#modules). 

### <a name='LEDStatusIndication'></a>LED Status Indication

The LED indicates the device status:

 *  On - Connected as client to WiFi network.
 *  Slow blinking - Opened an access point without password to connect to for setup.
 *  Blinking - Connecting to WiFi network using the given credentials.
 *  Fast blinking - Fatal error, MVP3000 not operational. This should never happen!

### <a name='WebInterface'></a>Web Interface

The main page lists basic system information and settings and links to all loaded modules. It also lists 

##### <a name='Network'></a>Network Settings

 *  Enter the network credentials of your local network in order to connect the ESP.
 *  Connect tries on boot before credentials are deemed wrong and an aceess point is opened. If the network connection was lost, the ESP will try to reconnect forever until restart.
 *  Force client mode on boot forever and do not fall back to opening an aceess point. \
 **WARNING:** If the credentials are wrong, the device will become inaccessible via the network! It can only be reset by re-flashing it!

### <a name='WebSockets'></a>WebSockets

WebSockets are provided mainly for the modules. An example for [log output](/examples/websocket/websocket_log.html) is available.

### <a name='MQTTCommunication'></a>MQTT Communication

MQTT is provided for the modules and not used by the framework itself. 

For more information on MQTT and developer resources also visit [Eclipse Paho](https://projects.eclipse.org/projects/iot.paho/developer).

##### Web Interface

 *  Connection status.
 *  The external broker overrides any discoverd local broker.
 *  MQTT port.
 *  List of active (_data) and subscribed (_ctrl) topics.


### <a name='UDPAutoDiscovery'></a>UDP Auto Discovery

UDP Auto Discovery allows to easily search the local network for other devices and servers, for example a MQTT server. There is no need to know device or server IPs in advance. Example Python scripts for the [server](/examples/udpdiscovery/server.py) and for [discovery](/examples/udpdiscovery/discover.py) are available.

One can also listen to UDP communication with netcat:

    nc -ukl [port]

##### Web Interface

 *  Discovered server, other ESP devices are not printed
 *  Port to use for discovery. This needs to be in accordance with the server-side pendant.


## <a name='Modules'></a>Modules

Typical use cases are available as modules to be loaded into the framework. First steps, examples, and options are given in the documentation of the respective module.

 *  [Sensor Module](/doc/sensor_module.md): The sensor module receives sensor data from the user script and processes it. It handles averaging, offset, scaling, and tare. Main settings are available via web interface. It publishes the live data to the serial console, to WebSocket, and via MQTT. The latter two additionally allow to 'zero' the measurement by the end-user. Stored data can be downloaded as CSV.
 *  LED Module (work in progress)
 *  Manipulator Module (planned)


## <a name='Implementation'></a>Implementation

### <a name='GeneralFunctionality'></a>General Functionality

A bare-bone implementation of the framework looks as follows. Without a [module](#modules) its usefulness is limited.

    #include <MVP3000.h>
    extern MVP3000 mvp;

    // Xmodule code
    // User code

    void setup() {
        // Xmodule code
        mvp.setup();

        // User code, mvp logging functionality is available
    }

    void loop() {
        // User code
        // Xmodule code
        mvp.loop();
    }

### <a name='NoBlockingdelayonESP'></a>No Blocking delay() on ESP

On ESP it is important to not use the blocking delay or while anywhere in the loop. This will impair the performance of the ESP, particularly the responsiveness of the network. As such it may significantly degrade the performance of the web interface and the output via WebSockets and MQTT.

Alternatives are to use the built-in [LimitTimer](/doc/helper_func.md#limittimer) or a custom method to wait using a timestamp variable.

### <a name='PublicMethodsandOptions'></a>Public Methods and Options

##### Selected Methods and Options

The serial output is color-coded using ANSI escape sequences. If your serial monitor does not support this feature (Arduino IDE) it can be turned off to omit the symbols. 

    mvp.logDisableAnsiColor();

The logger can be used to send message to serial and the websocket.

    mvp.log("This text will be timestamped and logged to serial, to websocket, and to the web interface.");
    mvp.logFormatted("This is a number: %d", 123);

##### Public Methods and Options

 *  `void addXmodule(_Xmodule *xmodule)`: Add a Xmodule to the MVP3000 system.
 *  `void log(const String& message)`: Log a message at 'user' level.
 *  `void logFormatted(const String& message, ...)`: Log a formatted message at 'user' level.
 *  `void logDisableAnsiColor()`: Disable ANSI codes in serial output.
 *  `void logSetLevel(CfgLogger::Level level)`: Change the logging level. The log level 'data' is only printed to serial and is omitted for the web page and WebSocket target.
 *  `void logSetTarget(CfgLogger::OutputTarget target, boolean enable)`: Enable/disable the output targets of logging message. Console and web interface are enabled by default, WebSocket is disabled.
 *  `void mqttHardDisable()`: Completely disable MQTT communication.
 *  `void udpHardDisable()`: Completely disable the UDP discovery service in case it interferes with custom UDP code.
 *  `void wsHardDisable()`: Completely disable WebSockets.
 *  `void setAlternateRoot(AwsResponseFiller alternateResponseFiller, AwsTemplateProcessor alternateTemplateProcessor = nullptr, const String& mvpUri = "/mvp3000")`: Set an alternate page as web root and move the main MVP3000 page to a sub-page.

### <a name='HelperFunctionsandClasses'></a>Helper Functions and Classes

Please see also the available [Helper Functions and Classes](/doc/helper_func.md).

### <a name='CustomModules'></a>Custom Modules

Most users will not need to create a custom module, particularly if a similar one already exists. We are happy to help implementing a missing feature.

In order to develop and integrate a new module into the MVP 3000 framework please follow the documentation to [Custom Modules](/doc/custom_modules.md). 


## <a name='ApplicationNote:CO2-LightsCO2-Ampel'></a>Application Note: CO2-Lights (CO2-Ampel)

The [CO2-Lights](https://github.com/Production3000/co2ampel) serves as a showcase application that highlights the versatility of the MVP3000 framework. This implementation uses the sensor module and the LED module. It features a custom landing page accessible via any smartphone or browser. The page provides real-time ambient CO2 concentration data alongside a historical graph, which updates automatically through WebSocket.


## <a name='Troubleshooting'></a>Troubleshooting

Q: I have problems accessing the web interface
A: Are you on the same WiFi network as the device? Do you have the correct IP, it is displayed in the serial log during startup.

Q: The web interface sometimes takes forever to respond.
A: Please check if there is any blocking delay or while in the loop. This significantly impairs user experience of all network related features on any ESP. Also check the libraries you use, particularly the sensor libraries often wait for the sensor to become ready.

Q: Some settings in my custom code are ignored?  
A: The value set during compile time is overwritten by stored values (if stored) during initialization. Possibly factory reset the device.


## <a name='Contribute'></a>Contribute

We are looking forward to your input on this project, be it bug reports, feature requests, or a successful implementation that does something cool. In the latter case we will gladly link you.

Please follow the [GitHub guide](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project) on how to contribute code to this project.


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
