# mesh_ftp_client
FTP client for esp-mesh-lite.   
Root node acts as an ftp client.   
Messages from leaf nodes are forwarded through the root node.   
```
+----------+               +-----------+                +-----------+
|ftp server|<--(ftp put)---| Root Node |<----(Mesh)-----| Leaf Node |
+----------+               +-----------+                +-----------+
```

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   
I used [this](https://github.com/nopnop2002/esp-idf-ftpClient) component.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G
- 1 x host computer connected to the network and running FTP server (optional)   
	You can use a public FTP server, such as ftp.dlptest.com.   

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_ftp_client
idf.py menuconfig
idf.py flash
```

# Configuration   
Set the information of your access point and your HTTP server.   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/218ee65f-d94c-403d-b576-dc82eb8db4e7)

## WiFi Setting
Set the information of your access point.   
![Image](https://github.com/user-attachments/assets/3c873f51-cb79-4ae2-b980-1d49ea2f9245)

## Server Setting
Set the information of your FTP server.   
The default server is [this](https://dlptest.com/ftp-test/) public server.   
The files will be stored for 10 minutes before being deleted.   
![Image](https://github.com/user-attachments/assets/54e845bf-9c3e-4db6-8fa6-7e17cdcaac79)

# Root node
The root node stores the file on the FTP server.
```
I (1443513) FTP-CLIENT: ftpClientPut /root/mesh-lite.txt ---> mesh-lite.txt
```


mesh-lite.txt contains information about the root node and leaf node.   
```
$ cat mesh-lite.txt
System information, channel: 1, layer: 1, self mac: a4:cf:12:05:c6:35, parent bssid: f8:b7:97:36:de:52, parent rssi: -53, free heap: 183012
System information, channel: 1, layer: 1, self mac: a4:cf:12:05:c6:35, parent bssid: f8:b7:97:36:de:52, parent rssi: -60, free heap: 173908
System information, channel: 1, layer: 2, self mac: c8:c9:a3:cf:10:c5, parent bssid: a4:cf:12:05:c6:35, parent rssi: -38, free heap: 182640
System information, channel: 1, layer: 1, self mac: a4:cf:12:05:c6:35, parent bssid: f8:b7:97:36:de:52, parent rssi: -60, free heap: 173568
System information, channel: 1, layer: 2, self mac: 3c:71:bf:9d:bd:01, parent bssid: a4:cf:12:05:c6:35, parent rssi: -47, free heap: 182260
System information, channel: 1, layer: 2, self mac: c8:c9:a3:cf:10:c5, parent bssid: a4:cf:12:05:c6:35, parent rssi: -33, free heap: 182620
```
