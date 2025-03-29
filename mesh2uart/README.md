# mesh2uart
This is esp-mesh-lite and UART/USB gateway application.
![Image](https://github.com/user-attachments/assets/12b3973e-7bcc-496a-a743-17dbaa0d66b7)

- Network configuration using a WiFi router   
	Connect to the host computer via a WiFi router.   
	```
	+-------------+             +-------------+          +-------------+          +-------------+
	|Host Computer|<--Network-->| WiFi router |<--WiFi-->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+             +-------------+          +-------------+          +-------------+
	```


- Network configuration for this project   
	Connect to the host computer via a UART/USB.   
	USB connection is only possible with ESP32S2/S3.   
	```
	+-------------+         +-----------------+          +-------------+          +-------------+
	|Host Computer|<--USB-->|USB-TTL CONVERTER|<--UART-->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+         +-----------------+          +-------------+          +-------------+

	+-------------+                                      +-------------+          +-------------+
	|Host Computer|<----------------USB----------------->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+                                      +-------------+          +-------------+
	```


# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards   
- If you use the UART interface, a USB-TTL converter   
- If you use the USB interface, a USB connector   
	I used this connector:
	![usb-connector](https://user-images.githubusercontent.com/6020549/124848149-3714ba00-dfd7-11eb-8344-8b120790c5c5.JPG)
	```
	ESP32-S2/S3 BOARD          USB CONNECTOR
	                           +--+
	                           | || VCC
	    [GPIO 19]    --------> | || D-
	    [GPIO 20]    --------> | || D+
	    [  GND  ]    --------> | || GND
	                           +--+
	```
- host computer capable of running terminal software   
- No WiFi router is required.

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh2uart
idf.py menuconfig
idf.py flash
```

# Configuration   
One device must be configured as the root node and the other devices as leaf nodes.   

- For Root node useing UART Interface   
	![Image](https://github.com/user-attachments/assets/bd3db0d5-45df-44b7-9dc4-256427cafb8d)

- For Root node useing USB Interface   
	![Image](https://github.com/user-attachments/assets/1fd642df-296a-42f5-ad51-47a377847c57)

- For Leaf node   
	![Image](https://github.com/user-attachments/assets/b7bdb3e2-3377-40d6-b919-6bbaf0145f75)


# Using Windows Terminal Software
When you connect the USB cable to the USB port on your Windows machine and build the firmware, a new COM port will appear.   
Open a new COM port in the terminal software.   
Input data from the keyboard is sent to all mesh nodes.   
I used TeraTerm.   
![Image](https://github.com/user-attachments/assets/6460ecaa-e6a5-4534-b7d2-10de02fefed9)

# Using Linux Terminal Software
When you connect the USB cable to the USB port on your Linux machine and build the firmware, a new /dev/tty device will appear.   
Open a new tty device in the terminal software.   
Most occasions, the device is /dev/ttyACM0.   
Input data from the keyboard is sent to all mesh nodes.   
I used cu.   
![Image](https://github.com/user-attachments/assets/914e5fbf-6f24-492f-b688-0002ecb0dcfa)

