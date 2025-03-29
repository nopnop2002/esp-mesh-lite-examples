# mesh2ble
This is esp-mesh-lite and BLE gateway application.   

![Image](https://github.com/user-attachments/assets/69b19953-a26d-41ad-b6fb-ca8abcf84a58)

- Network configuration using a WiFi router   
	Connect to the host computer via a WiFi router.   
	```
	+-------------+             +-------------+          +-------------+          +-------------+
	|Host Computer|<--Network-->| WiFi router |<--WiFi-->|  Root Node  |<--Mesh-->|  Leaf node  |
	+-------------+             +-------------+          +-------------+          +-------------+
	```


- Network configuration for this project   
	Connect to the host computer via a BLE.   
	```
	+-------------+                                      +-------------+          +-------------+
	| Android/iOS |<----------------BLE----------------->|  Root Node  |<--Mesh-->|  Leaf node  |
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
cd esp-mesh-lite-examples/mesh2ble
idf.py menuconfig
idf.py flash
```

# Configuration   
One device must be configured as the root node and the other devices as leaf nodes.   
![Image](https://github.com/user-attachments/assets/3136b9e4-ba93-431c-aa1c-7bf46d78d3d2)
![Image](https://github.com/user-attachments/assets/e799621a-98d1-4504-8b35-4687ce9ac3d1)


# How to use
ESP-IDF can use either the ESP-Bluedroid host stack or the ESP-NimBLE host stack.   
The differences between the two are detailed [here](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/ble/overview.html).   
This project uses the ESP-NimBLE host stack.   

- pair with ESP_NIMBLE_SERVER   

- Launch the app and select device  
Menu->Devices->Bluetooth LE   

- Long press the device and select the Edit menu   
![Image](https://github.com/user-attachments/assets/f89813cd-2db8-4740-a8bb-9f600672d7c0)

- Select Custom and specify UUID   
The UUIDs are different for ESP-Bluedroid and ESP-NimBLE.   
![Image](https://github.com/user-attachments/assets/fd9efc0c-37c8-448d-875a-015da2a9c1c8)

- Connect to device   
You can communicate to mesh-lite root node using android.   
![Image](https://github.com/user-attachments/assets/219104b8-259d-49b1-9e03-430071191f46)

# Concurrent connection
Unlike ESP-Bluedroid host stack, ESP-NimBLE host stack allows simultaneous connections.   
The maximum number of simultaneous connections is specified here.   
However, I don't own multiple Androids, so I haven't tried this.   
![Image](https://github.com/user-attachments/assets/9d1e1182-ed41-4b9e-bc55-bb3c75dd4745)

