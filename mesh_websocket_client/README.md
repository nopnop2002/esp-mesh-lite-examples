# mesh_websocket_client
WebSocket client for esp-mesh-lite.   
Every node acts as an web socket client.   
All nodes have the same IP address.   
```
+-------------+                +----------+
|             |<--(send text)--|   root   |
|             |                +----------+
|socket server|
|             |                +----------+
|             |<--(send text)--|   leaf   |
+-------------+                +----------+
```

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G
- 1 x host computer connected to the network and capable of running Python3

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_websocket_client
idf.py menuconfig
idf.py flash
```

# Configuration   
Set the information of your access point and your WebSocket server.   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/2d84d6d0-742d-457f-8857-8694647e41e5)

# How to use
Run ws-server.py on the host side.   
This script accepts requests from multiple clients.   
However, note that requests from esp-mesh-lite come from a single IP address.   
```
$ python3 ws-server.py
args.port=8080
message from ('192.168.10.119', 55525)=(324703) data from 3c:71:bf:9d:bd:00
message from ('192.168.10.119', 57656)=(371897) data from a4:cf:12:05:c6:34
message from ('192.168.10.119', 55525)=(325709) data from 3c:71:bf:9d:bd:00
message from ('192.168.10.119', 57656)=(372903) data from a4:cf:12:05:c6:34
```

