# mesh_local_control
esp-mesh-lite comes with [this](https://github.com/espressif/esp-mesh-lite/tree/master/examples/mesh_local_control) TCP client example.   
This is a TCP server script that can communicate with multiple ESP32s.   
You can try esp-mesh-lite now.   

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
cd esp-mesh-lite/examples/mesh_local_control
idf.py menuconfig
idf.py flash
```


# How to use
This is a log of communication with two ESP32s.   
It shows their respective MAC addresses and MESH layers.   
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

