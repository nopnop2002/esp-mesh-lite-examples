# mesh_remote_logging
ESP-MESH-LITE logging is displayed on STDOUT of each node.   
When troubleshooting, you need to collect logging from all nodes.   
This project displays logging from all nodes in one place.   
This project is based on [this](https://github.com/espressif/esp-mesh-lite/tree/master/examples/mesh_local_control) official example.   
This project acts as a TCP client, just like the official example.   

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G
- 1 x host computer connected to the network and capable of running Python3

# Installation
```
git clone https://github.com/espressif/esp-mesh-lite
cd esp-mesh-lite/examples/mesh_remote_logging
idf.py menuconfig
idf.py flash
```

# Configuration
![Image](https://github.com/user-attachments/assets/e2f8a337-0da2-4d04-bc13-edcee66e7d72)
![Image](https://github.com/user-attachments/assets/d86143b1-ddb8-40fb-aae5-63b129a068c8)

# Start TCP Server
```
$ python3 tcp-server-threading.py
port=8070
Connected from ('192.168.10.115', 56558)
on_new_client
Connected from ('192.168.10.115', 54088)
on_new_client
msg={"src_addr": "3c:71:bf:9d:bd:00","data": "Hello TCP Server!","level": 1,"count": 143}
msg={"src_addr": "a4:cf:12:05:c6:34","data": "Hello TCP Server!","level": 2,"count": 202}
msg={"src_addr": "3c:71:bf:9d:bd:00","data": "Hello TCP Server!","level": 1,"count": 144}
msg={"src_addr": "a4:cf:12:05:c6:34","data": "Hello TCP Server!","level": 2,"count": 203}
```

# View Logging using python
```
$ python ./udp-server.py
args.port=8090
+==========================+
| ESP32 UDP Logging Server |
+==========================+

a4:cf:12:05:c6:34>I (4517) MAIN: TCP client write task is running
a4:cf:12:05:c6:34>I (5021) MAIN: Create a tcp client, ip: 192.168.10.46, port: 8070
a4:cf:12:05:c6:34>I (8030) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (8234) wifi:a4:cf:12:05:c6:34>new:<1,1>, old:<1,1>, ap:<1,1>, sta:<1,0>, prof:1, snd_ch_cfg:0x0a4:cf:12:05:c6:34>
a4:cf:12:05:c6:34>I (8236) wifi:a4:cf:12:05:c6:34>station: c8:c9:a3:cf:10:c4 join, AID=1, bgn, 40Ua4:cf:12:05:c6:34>
a4:cf:12:05:c6:34>I (8259) bridge_wifi: STA Connecting to the AP again...
a4:cf:12:05:c6:34>I (8296) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.5.2
c8:c9:a3:cf:10:c4>I (4444) MAIN: TCP client write task is running
c8:c9:a3:cf:10:c4>I (4949) MAIN: Create a tcp client, ip: 192.168.10.46, port: 8070
a4:cf:12:05:c6:34>I (11032) MAIN: TCP write, size=83 ret=83
c8:c9:a3:cf:10:c4>I (7960) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (14036) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (15179) wifi:a4:cf:12:05:c6:34>new:<1,1>, old:<1,1>, ap:<1,1>, sta:<1,0>, prof:1, snd_ch_cfg:0x0a4:cf:12:05:c6:34>
a4:cf:12:05:c6:34>I (15181) wifi:a4:cf:12:05:c6:34>station: 3c:71:bf:9d:bd:00 join, AID=2, bgn, 40Ua4:cf:12:05:c6:34>
a4:cf:12:05:c6:34>I (15204) bridge_wifi: STA Connecting to the AP again...
a4:cf:12:05:c6:34>I (15233) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.5.3
c8:c9:a3:cf:10:c4>I (10963) MAIN: TCP write, size=83 ret=83
c8:c9:a3:cf:10:c4>W (11174) wifi:c8:c9:a3:cf:10:c4>Password length matches WPA2 standards, authmode threshold changes from OPEN to WPA2c8:c9:a3:cf:10:c4>
c8:c9:a3:cf:10:c4>I (11185) [vendor_ie]: set router_config [aterm-d5a4ee-g]
3c:71:bf:9d:bd:00>I (4337) MAIN: TCP client write task is running
3c:71:bf:9d:bd:00>I (4841) MAIN: Create a tcp client, ip: 192.168.10.46, port: 8070
a4:cf:12:05:c6:34>I (17039) MAIN: TCP write, size=83 ret=83
c8:c9:a3:cf:10:c4>I (13966) MAIN: TCP write, size=83 ret=83
3c:71:bf:9d:bd:00>I (7851) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (20042) MAIN: TCP write, size=83 ret=83
c8:c9:a3:cf:10:c4>I (16969) MAIN: TCP write, size=83 ret=83
3c:71:bf:9d:bd:00>I (10853) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (23045) MAIN: TCP write, size=83 ret=83
3c:71:bf:9d:bd:00>W (11163) wifi:3c:71:bf:9d:bd:00>Password length matches WPA2 standards, authmode threshold changes from OPEN to WPA23c:71:bf:9d:bd:00>
3c:71:bf:9d:bd:00>I (11182) [vendor_ie]: set router_config [aterm-d5a4ee-g]
c8:c9:a3:cf:10:c4>I (19970) MAIN: TCP write, size=83 ret=83
3c:71:bf:9d:bd:00>I (13856) MAIN: TCP write, size=83 ret=83
a4:cf:12:05:c6:34>I (26048) MAIN: TCP write, size=83 ret=83
```

# View Logging using Windows Application
We can also use [this](https://apps.microsoft.com/detail/9p4nn1x0mmzr?hl=ja-JP&gl=JP) as Logging Viewer.   
![Image](https://github.com/user-attachments/assets/4405c2c9-c1e9-4afb-91af-6bd31e68b5ad)


# API
```
    // Start remote logging
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    char mac[20];
    snprintf(mac, sizeof(mac), MACSTR, MAC2STR(sta_mac));
    remote_logging_init(mac, 1);
```
