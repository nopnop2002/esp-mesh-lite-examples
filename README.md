# esp-mesh-lite-examples
The release version of esp-mesh-lite is available [here](https://components.espressif.com/components/espressif/mesh_lite/).   
A development version of esp-mesh-lite is available [here](https://github.com/espressif/esp-mesh-lite).   

You can create a wide-area mesh network.   
The official repository comes with some example code.   
The official examples are very helpful when creating an application for esp-mesh-lite.   
This repository contains example code for esp-mesh-lite that is not in the official repository.   

# The differences between ESP-MESH-LITE and ESP-MESH
ESP-MESH is a standard feature of ESP-IDF.   
An ESP-MESH example is available [here](https://github.com/espressif/esp-idf/tree/master/examples/mesh).   
ESP-MESH and ESP-MESH-LITE have different architectures.   

The ESP-MESH-LITE documentation is available [here](https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/User_Guide.md).   
Among them, the following is particularly important:   
```Each node in ESP-Mesh-Lite enables the LWIP stack and can be treated as a device directly connected to the router, which can independently invoke network interfaces such as Socket, MQTT, HTTP, etc., on the application layer.```   
This makes communication with external networks incredibly easy.   

MESH-LITE uses [this](https://github.com/espressif/esp-iot-bridge/blob/master/components/iot_bridge/User_Guide.md) esp-iot-bridge.   
[Here](https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/wifi_router_en.png) is a typical block diagram of this technology.

# Mesh Solutions
In addition to ESP-MESH-LITE, we can use the following Mesh Solutions:   
Mesh Solution Comparison is [here](https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/solution-introduction/mesh/mesh-comparison.html).   
- ESP BLE Mesh   
- ESP Thread Mesh   
- ZigBee Mesh   
- ESP-Now   


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

# router_less_message_exchange
An example of a routerless mesh network that exchanges data between nodes.   
esp-mesh-lite comes with [this](https://github.com/espressif/esp-mesh-lite/tree/master/examples/no_router) router less example.   
However, this example does not have communication functionality between nodes.   

# mesh2uart
esp-mesh-lite and UART/USB gateway application for routerless mesh networks.

# mesh2ble
esp-mesh-lite and BLE gateway application for routerless mesh networks.   

# Restructuring network topology   

### Loss of a leaf node   
If communication from a leaf node to the root node is lost for a certain period of time, the root node will determine that the leaf node is lost.   
Then, it will disconnect the leaf node from the MESH network.   
This is the logging of the root node at that time.   
```
W (4296140) wifi:inactive timer: now=713bb last_rx_time=ee0e5bf3 diff=499c8, aid[2]3c:71:bf:9d:bd:00 leave
I (4296164) wifi:station: 3c:71:bf:9d:bd:00 leave, AID = 2, reason = 4, bss_flags is 753779, bss:0x3ffd22dc
I (4296165) wifi:new:<11,2>, old:<11,2>, ap:<11,2>, sta:<0,0>, prof:11, snd_ch_cfg:0x0
E (4296171) bridge_wifi: STA Disconnect to the AP
W (4297880) wifi:inactive timer: now=21a180 last_rx_time=ee2ea74b diff=49850, aid[1]c8:c9:a3:cf:10:c4 leave
I (4297905) wifi:station: c8:c9:a3:cf:10:c4 leave, AID = 1, reason = 4, bss_flags is 753779, bss:0x3ffba868
I (4297906) wifi:new:<11,0>, old:<11,2>, ap:<11,2>, sta:<0,0>, prof:11, snd_ch_cfg:0x0
E (4297913) bridge_wifi: STA Disconnect to the AP
```

### Loss of a root node   
If the root node is lost for any reason, one of the leaf nodes is promoted to root node.   
This is the logging of the root node at that time.   
This node was in layer 2 until tick 91204, but was promoted to the root node from tick 95683 onwards.   
```
I (61204) local_control: System information, channel: 1, layer: 2, self mac: 08:3a:f2:50:de:5c, parent bssid: a4:cf:12:05:c6:35, parent rssi: -46, free heap: 180504
I (71204) local_control: System information, channel: 1, layer: 2, self mac: 08:3a:f2:50:de:5c, parent bssid: a4:cf:12:05:c6:35, parent rssi: -47, free heap: 180504
I (81204) local_control: System information, channel: 1, layer: 2, self mac: 08:3a:f2:50:de:5c, parent bssid: a4:cf:12:05:c6:35, parent rssi: -46, free heap: 180504
I (91204) local_control: System information, channel: 1, layer: 2, self mac: 08:3a:f2:50:de:5c, parent bssid: a4:cf:12:05:c6:35, parent rssi: -56, free heap: 178808
I (95683) wifi:bcn_timeout,ap_probe_send_start
I (98186) wifi:ap_probe_send over, reset wifi status to disassoc
I (98187) wifi:state: run -> init (0xc800)
I (98189) wifi:pm stop, total sleep time: 0 us / 94021266 us

I (98191) wifi:<ba-del>idx:0, tid:0
I (98195) wifi:new:<1,0>, old:<1,1>, ap:<1,1>, sta:<1,1>, prof:1, snd_ch_cfg:0x0
E (98207) [ESP_Mesh_Lite_Comm]: Disconnect reason : 200
E (98801) local_control: <Software caused connection abort> TCP write
W (99044) wifi:Password length matches WPA2 standards, authmode threshold changes from OPEN to WPA2
I (99047) [vendor_ie]: esp_wifi_connect return ESP_OK
I (99049) [vendor_ie]: RTC store: temp_mesh_id:115; ssid:aterm-d5a4ee-g; bssid:00:00:00:00:00:00; crc:3383892968
I (99057) [ESP_Mesh_Lite_Comm]: As the first inherited device, try to connect to the router
I (99058) wifi:new:<1,1>, old:<1,0>, ap:<1,1>, sta:<1,0>, prof:1, snd_ch_cfg:0x0
I (99074) wifi:state: init -> auth (0xb0)
I (99080) wifi:state: auth -> assoc (0x0)
I (99105) wifi:state: assoc -> run (0x10)
I (99210) wifi:connected with aterm-d5a4ee-g, aid = 15, channel 1, BW20, bssid = f8:b7:97:36:de:52
I (99211) wifi:security: WPA2-PSK, phy: bgn, rssi: -53
I (99214) wifi:pm start, type: 1

I (99295) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (100354) esp_netif_handlers: sta ip: 192.168.10.121, mask: 255.255.255.0, gw: 192.168.10.1
I (100355) bridge_wifi: Connected with IP Address:192.168.10.121
I (100359) [vendor_ie]: RTC store: temp_mesh_id:121; ssid:aterm-d5a4ee-g; bssid:00:00:00:00:00:00; crc:2665160230
I (101143) wifi:<ba-add>idx:0 (ifx:0, f8:b7:97:36:de:52), tid:5, ssn:0, winSize:64
I (101204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -55, free heap: 180284
I (101247) wifi:<ba-add>idx:1 (ifx:0, f8:b7:97:36:de:52), tid:0, ssn:6, winSize:64
I (111204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -55, free heap: 180484
```

