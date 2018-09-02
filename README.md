Internet of Things internship project written in C/C++

Noise Detector helps to work in healthy enviroment. Noise Detector is a simple and efficient way of lowering noise levels in the office or public places. It is easy to move from one place to another and get measurements in the new space.

No software installation is required. Just turn on and connect to your WiFi Network. Immediately after connecting, the device starts to send the measured noise level to a cloud service enabling analytics for your working environment, even without invasive notifications.

During internship I have got the practical experience wuth ESP32 Microcontroller, WiFi connectivity and MQTT cloud service communication, sensors integration, SPI, UART, PWM, ADC, multimeter and oscilloscope. I have designed PCB board in Eagle CAD, ordered components with BOM.

ESP32-DevKitC is used for Noise Click Project as an entry-level development board. All its GPIO pins are exposed and this allows programming of all necessary functions and connect with sound sensor, LED and buzzer sub-systems.

WITH FREE PROFESSIONAL REAL TIME OPERATING SYSTEM FOR MICROCONTROLLERS
In order to fulfil its goal of noise level monitoring, Noise Detector has to perform several parallel tasks, sending information to the cloud and producing active notifications to coworkers. FreeRTOS (free realtime operating system) allowed to implement this functionality using the Espressif toolchain.


