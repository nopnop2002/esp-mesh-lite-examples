# mesh_ftp_client
FTP client for esp-mesh-lite.   
Every node acts as an ftp client.   
All nodes have the same IP address.   
I used [this](https://github.com/nopnop2002/esp-idf-ftpClient) component.   
```
+----------+               +----------+
|          |<--(ftp put)---|   root   |
|          |               +----------+
|ftp server|
|          |               +----------+
|          |<--(ftp get)---|   leaf   |
+----------+               +----------+
```

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

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
I (1442286) CLIENT: System information, channel: 11, layer: 1, self mac: a4:cf:12:05:c6:34, parent bssid: f8:b7:97:36:de:52, parent rssi: -58, free heap: 165564
I (1443513) CLIENT: ftpClientPut /root/mesh-lite.txt ---> mesh-lite.txt
```

# Leaf node
The leaf node retrieves the file from the FTP server.
```
I (65399) CLIENT: ftpClientGet /root/mesh-lite.txt <--- mesh-lite.txt
I (65401) CLIENT: -----------------------------------------------
I (65404) CLIENT: System information
I (65407) CLIENT: channel: 11
I (65411) CLIENT: layer: 1
I (65414) CLIENT: self mac: a4:cf:12:05:c6:34
I (65419) CLIENT: parent bssid: f8:b7:97:36:de:52
I (65425) CLIENT: parent rssi: -61free heap: 163812
I (65430) CLIENT: Child mac: 3c:71:bf:9d:bd:00
I (65435) CLIENT: Child mac: 24:0a:c4:c5:46:fc
I (65441) CLIENT: -----------------------------------------------
```

This warning appears when a leaf node retrieves a file while the root node is storing the file.   
```
I (1973647) CLIENT: ftpClientGet /root/mesh-lite.txt <--- mesh-lite.txt
W (1973681) CLIENT: Failed to open file for reading
```
