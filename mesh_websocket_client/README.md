# mesh_websocket_client
WebSocket client for esp-mesh-lite.   
Every node acts as an web socket client.   
All nodes have the same IP address.   
Messages from leaf nodes are forwarded through the root node.   
```
                                       ESP32
+-------------+                    +-------------+
|             |<--SOCKET packets-->|SOCKET client|
|             |                    |     root    |                        ESP32
|SOCKET server|                    |             |                    +-------------+
|             |                    |             |                    |SOCKET client|
|             |<--SOCKET packets-->|<===========>|<--SOCKET packets-->|     leaf    |
+-------------+                    +-------------+                    +-------------+
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
$ python3 -m pip install websockets
$ python3 ws-server.py
args.port=8080
message from ('192.168.10.131', 49219)={"level":1,"mac":"24:0a:c4:c5:46:fc","now":829103,"cores":2,"target":"esp32"}
message from ('192.168.10.131', 52162)={"level":3,"mac":"60:55:f9:79:80:94","now":203425,"cores":1,"target":"esp32c3"}
message from ('192.168.10.131', 57941)={"level":3,"mac":"10:97:bd:f3:61:f0","now":9216,"cores":1,"target":"esp32c2"}
message from ('192.168.10.131', 50741)={"level":2,"mac":"80:65:99:ea:ff:00","now":566530,"cores":1,"target":"esp32s2"}
```

