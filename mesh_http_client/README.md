# mesh_http_client
HTTP client for esp-mesh-lite.

# Installation

```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_http_client
idf.py menuconfig
idf.py flash
```

# Configuration   
Set the information of your access point and your HTTP server.   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/2d84d6d0-742d-457f-8857-8694647e41e5)

# How to use
Run http-server.py on the host side.   
This script accepts requests from multiple MAC addresses.   
However, note that the response is to a single IP address.   
```
$ python3 http-server.py
args.port=8080
(766517) data from a4:cf:12:05:c6:34
192.168.10.119 - - [24/Mar/2025 14:53:14] "POST / HTTP/1.1" 200 -
(706694) data from 3c:71:bf:9d:bd:00
192.168.10.119 - - [24/Mar/2025 14:53:14] "POST / HTTP/1.1" 200 -
(767554) data from a4:cf:12:05:c6:34
192.168.10.119 - - [24/Mar/2025 14:53:15] "POST / HTTP/1.1" 200 -
(707748) data from 3c:71:bf:9d:bd:00
192.168.10.119 - - [24/Mar/2025 14:53:15] "POST / HTTP/1.1" 200 -
(768590) data from a4:cf:12:05:c6:34
192.168.10.119 - - [24/Mar/2025 14:53:16] "POST / HTTP/1.1" 200 -
```

