# esp-mesh-lite-examples
esp-mesh-lite is available [here](https://github.com/espressif/esp-mesh-lite).   
You can create a wide-area mesh network.   
The official repository comes with some example code.   
The official examples are very helpful when creating an application for esp-mesh-lite.   
This repository contains example code for esp-mesh-lite that is not in the official repository.   

# The differences between ESP-MESH-LITE and ESP-MESH
ESP-MESH is a standard feature of ESP-IDF.   
An ESP-MESH example is available [here](https://github.com/espressif/esp-idf/tree/master/examples/mesh).   
The differences between ESP-MESH-LITE and ESP-MESH are published [here](https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/User_Guide.md#difference-between-esp-mesh-lite-and-esp-mesh).   
Among them, the following is particularly important:   
```In ESP-MESH, only the root node enables the LWIP stack, and all child nodes must transfer via the root node to communicate with external networks.```   
To put it the other way around,   
```In ESP-MESH-LITE, all nodes enables the LWIP stack, and all child nodes don't have to transfer via the root node to communicate with external networks.```   
This makes communication with external networks incredibly easy.   
ESP-MESH-LITE utilizes [this](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md) technology.   
[Here](https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wifi_router_en.png) is a typical block diagram of this function.
# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# mesh_mqtt_client
MQTT client using esp-mesh-lite.   

# mesh_sntp_client
SNTP client using esp-mesh-lite.   

# mesh_http_client
HTTP client using esp-mesh-lite.   

# mesh_http_server
HTTP server using esp-mesh-lite.   

# mesh_websocket_client
WebSocket client using esp-mesh-lite.   

# mesh_websocket_server
WebSocket server using esp-mesh-lite.   

# mesh_ftp_client
FTP client using esp-mesh-lite.   

# mesh_local_control
esp-mesh-lite comes with [this](https://github.com/espressif/esp-mesh-lite/tree/master/examples/mesh_local_control) TCP client example.   
However, it does not include a server-side script.   
This is a TCP server script that can communicate with multiple ESP32s.   
You can try esp-mesh-lite now.   

# no_router_message_exchange
An example of a routerless mesh network that exchanges data between nodes.   

# mesh2uart
esp-mesh-lite and UART/USB gateway application for routerless mesh networks.

# mesh2ble
esp-mesh-lite and BLE gateway application for routerless mesh networks.   
