# mesh_http_server
HTTP server for esp-mesh-lite.

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G
- 1 x host computer connected to the network and capable of running browser

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_http_server
idf.py menuconfig
idf.py flash
```

# Configuration for root   
One device must be configured as the root node and the other devices as leaf nodes.   
The root node acts as a WebServer.   
Set the information of your access point and WebServer port.   
![Image](https://github.com/user-attachments/assets/24ab6d3b-a3f6-4e8b-b33f-cf570dbcd103)
![Image](https://github.com/user-attachments/assets/fcda0877-fc5b-462b-8e2c-7c48042da6ee)

# Configuration for leaf   
Leaf nodes communicate with the root node over WiFi.   
The root node and leaf nodes must use the same WiFi channel.   
The WiFi channel used by the root node is obtained from the AP.   
![Image](https://github.com/user-attachments/assets/24ab6d3b-a3f6-4e8b-b33f-cf570dbcd103)
![Image](https://github.com/user-attachments/assets/fba33f9e-958a-4914-a50d-b4476037efb4)

# How to use
Open a browser and enter the IP address of the ESP32 in the address bar.   
Displays all node information in the Meh-Network.   
![Image](https://github.com/user-attachments/assets/3922b323-79cf-4a89-b518-9fcdf40a568f)

You can use the mDNS hostname instead of the IP address.   
![Image](https://github.com/user-attachments/assets/683df27d-f4cc-417e-9962-e12a0af9d4e4)
