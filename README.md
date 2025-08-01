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

### Loss of a child node   
If a child node is lost for any reason, the parent node disconnects the child node from the MESH network.   
This is the logging of the parent node at that time.   
The parent node has two child nodes up to tick 481219.   
From tick 486019 onwards, the parent node has one child node.
```
I (481219) no_router: System information, channel: 11, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: 00:00:00:00:00:00, parent rssi: -120, free heap: 181260
I (481219) no_router: Child mac: 24:0a:c4:c5:46:fc
I (481219) no_router: Child mac: a4:cf:12:05:c6:34
W (486019) wifi:inactive timer: now=1cec87a2 last_rx_time=b0346d1 diff=495d3, aid[2]a4:cf:12:05:c6:34 leave
I (486049) wifi:station: a4:cf:12:05:c6:34 leave, AID = 2, reason = 4, bss_flags is 756851, bss:0x3ffbad54
I (486049) wifi:new:<11,2>, old:<11,2>, ap:<11,2>, sta:<0,0>, prof:11, snd_ch_cfg:0x0
I (486049) wifi:<ba-del>idx:3, tid:0
E (486059) bridge_wifi: STA Disconnect to the AP
I (491219) no_router: System information, channel: 11, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: 00:00:00:00:00:00, parent rssi: -120, free heap: 183456
I (491219) no_router: Child mac: 24:0a:c4:c5:46:fc
I (501219) no_router: System information, channel: 11, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: 00:00:00:00:00:00, parent rssi: -120, free heap: 183456
I (501219) no_router: Child mac: 24:0a:c4:c5:46:fc
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

### Adding leaf nodes   
When a leaf node is added, the root node or leaf node automatically adds it to the network.   
The parent node is determined automatically based on the surrounding circumstances.   
This is the logging of the root node at that time.   
The root node has one child node up to tick 1031209.    
From tick 1038584 onwards, the root node has two child nodes.   
```
I (1021204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -56, free heap: 178116
I (1021211) local_control: Child mac: 24:0a:c4:c5:46:fc
I (1031204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -56, free heap: 178116
I (1031209) local_control: Child mac: 24:0a:c4:c5:46:fc
I (1038584) wifi:new:<1,1>, old:<1,1>, ap:<1,1>, sta:<1,0>, prof:1, snd_ch_cfg:0x0
I (1038585) wifi:station: a4:cf:12:05:c6:34 join, AID=2, bgn, 40U
I (1038614) bridge_wifi: STA Connecting to the AP again...
I (1038637) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.4.3
I (1039679) wifi:<ba-add>idx:3 (ifx:1, a4:cf:12:05:c6:34), tid:0, ssn:0, winSize:64
I (1041204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -56, free heap: 175748
I (1041209) local_control: Child mac: 24:0a:c4:c5:46:fc
I (1041214) local_control: Child mac: a4:cf:12:05:c6:34
I (1051204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -56, free heap: 175748
I (1051209) local_control: Child mac: 24:0a:c4:c5:46:fc
I (1051214) local_control: Child mac: a4:cf:12:05:c6:34
I (1061204) local_control: System information, channel: 1, layer: 1, self mac: 08:3a:f2:50:de:5c, parent bssid: f8:b7:97:36:de:52, parent rssi: -57, free heap: 175560
I (1061209) local_control: Child mac: 24:0a:c4:c5:46:fc
I (1061214) local_control: Child mac: a4:cf:12:05:c6:34
```