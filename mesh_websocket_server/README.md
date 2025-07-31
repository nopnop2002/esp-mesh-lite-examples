# mesh_websocket_server
WebSocket server for esp-mesh-lite.   
Only the root node can act as a server.   
Leaf nodes communicate with the root node via internal communication.   
```
                                ESP32                        ESP32
+----------+              +-------------+               +----------+
|          |---request--->|SOCKET server|               |          |
| Browser  |              |     root    |<--(Internal)--|   leaf   |
|          |<--response---|             |               |          |
+----------+              +-------------+               +----------+
```

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
cd esp-mesh-lite-examples/mesh_websocket_server
idf.py menuconfig
idf.py flash
```

# Configuration for root   
One device must be configured as the root node and the other devices as leaf nodes.   
The root node acts as a WebSocket Server.   
![Image](https://github.com/user-attachments/assets/a1fcd3b5-e19b-4098-b504-4187ea2188fc)
![Image](https://github.com/user-attachments/assets/dc497f42-0683-458e-a5f7-221631145efe)

# Configuration for leaf   
Leaf nodes communicate with the root node over WiFi.   
The root node and leaf nodes must use the same WiFi channel.   
The WiFi channel used by the root node is obtained from the AP.   
![Image](https://github.com/user-attachments/assets/d59920b5-57af-4434-aa9b-ebd6c60d4143)
![Image](https://github.com/user-attachments/assets/9b523ac1-d62c-473f-864d-56cf2d70ccf2)

# How to use
Open a browser and enter the IP address of the ESP32 in the address bar.   
Displays all node information in the Meh-Network.   
![Image](https://github.com/user-attachments/assets/d417abaa-0884-4a98-b07a-79ac03d33a9d)
![Image](https://github.com/user-attachments/assets/a50b74fd-7fcd-4048-94a5-d2c097a580ba)
![Image](https://github.com/user-attachments/assets/c0bfc630-31a5-4272-b8b9-628e267e07e0)
![Image](https://github.com/user-attachments/assets/821abce2-f542-4380-886a-5de3ab6d9daf)
![Image](https://github.com/user-attachments/assets/91bf29a5-3bbe-4803-bd6e-d45e7a35cba9)


console logging:
```
I (703805) NODES: nodes[0] level=1 self_mac=[a4:cf:12:05:c6:35] parent_mac=[00:00:00:00:00:00]
I (703815) NODES: nodes[1] level=2 self_mac=[80:65:99:ea:fe:e9] parent_mac=[a4:cf:12:05:c6:35]
I (703825) NODES: nodes[2] level=2 self_mac=[c0:4e:30:4b:12:13] parent_mac=[a4:cf:12:05:c6:35]
I (703835) NODES: nodes[3] level=3 self_mac=[60:55:f9:79:b1:41] parent_mac=[80:65:99:ea:fe:e9]
I (703845) NODES: nodes[4] level=4 self_mac=[60:55:f9:75:ca:9d] parent_mac=[60:55:f9:79:b1:41]
I (703855) NODES: nodes[5] level=4 self_mac=[80:65:99:e1:35:45] parent_mac=[60:55:f9:79:b1:41]
I (703865) NODES: nodes[6] level=5 self_mac=[60:55:f9:79:80:95] parent_mac=[60:55:f9:75:ca:9d]
```

This shows the following mesh network configuration:
```
<---- level 1 ---->     <---- level 2 ---->     <---- level 3 ---->     <---- level 4 ---->     <---- level 5 ---->
+-----------------+     +-----------------+     +-----------------+     +-----------------+     +-----------------+
|a4:cf:12:05:c6:35|--+--|80:65:99:ea:fe:e9|-----|60:55:f9:79:b1:41|--+--|60:55:f9:75:ca:9d|-----|60:55:f9:79:80:95|
+-----------------+  |  +-----------------+     +-----------------+  |  +-----------------+     +-----------------+
                     |                                               |
                     |                                               |  +-----------------+
                     |                                               +--|80:65:99:e1:35:45|
                     |                                                  +-----------------+
                     |  +-----------------+
                     +--|c0:4e:30:4b:12:13|
                        +-----------------+
```

You can use the mDNS hostname instead of the IP address.   
![Image](https://github.com/user-attachments/assets/b188cbfb-16e7-4cdc-8822-b0055c543add)
