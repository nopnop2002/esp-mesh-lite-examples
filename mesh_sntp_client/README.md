# mesh_sntp_client
SNTP client for esp-mesh-lite.

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_sntp_client
idf.py menuconfig
idf.py flash
```

# Configuration   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/95a4a071-77f0-48b2-82bf-cfff456f59c7)

## WiFi Setting
Set the information of your access point.   
![Image](https://github.com/user-attachments/assets/9c68c775-4970-4a47-b15a-2fb96521060a)

## NTP Setting
Set the information of your NTP server.   
![Image](https://github.com/user-attachments/assets/3a30427f-a496-493a-9abc-fd8aa37bf70c)


# Root node logging
```
I (681270) MAIN: System information, channel: 11, layer: 1, self mac: a4:cf:12:05:c6:34, parent bssid: f8:b7:97:36:de:52, parent rssi: -57, free heap: 179956
I (681275) MAIN: Child mac: 3c:71:bf:9d:bd:00
I (686209) MAIN: The current date/time is: Mon Mar 24 09:27:53 2025
```


# Leaf node logging
```
I (651278) MAIN: System information, channel: 11, layer: 2, self mac: 3c:71:bf:9d:bd:00, parent bssid: a4:cf:12:05:c6:35, parent rssi: -42, free heap: 181872
I (658152) MAIN: The current date/time is: Mon Mar 24 09:29:04 2025
```