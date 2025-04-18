# mesh2can
This is esp-mesh-lite and CAN gateway application.   

![Image](https://github.com/user-attachments/assets/486d79d5-b116-46a1-9336-6ad5a1460e53)

- Network configuration using a WiFi router   
	Connect to the host computer via a WiFi router.   
	```
	+-------------+             +-------------+          +-------------+          +-------------+
	|Host Computer|<--Network-->| WiFi router |<--WiFi-->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+             +-------------+          +-------------+          +-------------+
	```


- Network configuration for this project   
	Connect to the host computer via a CAN.   
	```
	+-------------+                                      +-------------+          +-------------+
	|Host Computer|<----------------CAN----------------->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+                                      +-------------+          +-------------+
	```


# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards   
- A BLE client like a smartphone   
- No WiFi router is required.

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh2can
idf.py menuconfig
idf.py flash
```

# Configuration   
One device must be configured as the root node and the other devices as leaf nodes.   
- For Root node   
	![Image](https://github.com/user-attachments/assets/06584d50-ff8f-45db-a5d2-0b07d9c32ce3)
	![Image](https://github.com/user-attachments/assets/db6dedcd-755a-4541-bb97-f7a9ad511f27)

- For leaf node using standard can frame   
	Leaf node information is notified using the standard can frame.   
	![Image](https://github.com/user-attachments/assets/06584d50-ff8f-45db-a5d2-0b07d9c32ce3)
	![Image](https://github.com/user-attachments/assets/bd52933e-0f21-4bfc-8443-54f9511bfb38)

- For leaf node using extended can frame   
	Leaf node information is notified using the extended can frame.   
	![Image](https://github.com/user-attachments/assets/06584d50-ff8f-45db-a5d2-0b07d9c32ce3)
	![Image](https://github.com/user-attachments/assets/04806a7d-9fd9-4325-adbd-18ab7b38c912)

# CAN-BUS monitoring
You can use this to monitor the CAN-BUS.   
https://github.com/nopnop2002/esp-idf-candump   
https://github.com/nopnop2002/Arduino-CANBus-Monitor   
https://github.com/nopnop2002/esp-idf-CANBus-Monitor   

The maximum message length for CAN is 8 bytes.   
The following 8 bytes are notified from the leaf node.   
- Byte 0   
	Mesh level   
- Byte 1   
	Sequence number   
- Byte 2-7   
	MAC address of leaf node.

Each leaf node uses a separate CAN-ID.   
The CAN-ID is configured using menuconfig.   
![Image](https://github.com/user-attachments/assets/df1ad5db-9c42-4a1c-919a-b40ab8d21d5d)


# CAN data forwarding   
All CAN messages received by the root node are forwarded to all leaf nodes.   
```
+-------------+                                      +-------------+          +-------------+
|OtherComputer|-----------------CAN----------------->|  Root Node  |---Mesh-->|  Leaf node  |
+-------------+                                      +-------------+          +-------------+
```
![Image](https://github.com/user-attachments/assets/7d34231d-4cfa-4772-aa24-c01fabc340a0)
