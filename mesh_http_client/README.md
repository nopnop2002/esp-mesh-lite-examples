# mesh_http_client
HTTP client for esp-mesh-lite.   
Every node acts as an http client.   
All nodes have the same IP address.   
Messages from leaf nodes are forwarded through the root node.   
```
                               ESP32
+-----------+              +-----------+
|           |<--HTTP post--|HTTP client|
|           |              |   root    |                  ESP32
|HTTP server|              |           |              +-----------+
|           |              |           |              |HTTP client|
|           |<--HTTP post--|<----------|<--HTTP post--|   leaf    |
+-----------+              +-----------+              +-----------+
```

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards
- 1 x router that supports 2.4G
- 1 x host computer connected to the network and capable of running Python3

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
![Image](https://github.com/user-attachments/assets/048573f9-83c6-4c20-9911-69496e9c807b)

# How to use
Run http-server.py on the host side.   
This script accepts requests from multiple clients.   
However, note that requests from esp-mesh-lite come from a single IP address.   
```
$ python3 http-server.py
args.port=8080
message from ('192.168.10.154', 60382)={"level":2,"mac":"24:0a:c4:c5:46:fc","now":242027,"cores":2,"target":"esp32"}
message from ('192.168.10.154', 55237)={"level":2,"mac":"60:55:f9:79:80:94","now":198881,"cores":1,"target":"esp32c3"}
message from ('192.168.10.154', 52865)={"level":2,"mac":"10:97:bd:f3:61:f0","now":24747,"cores":1,"target":"esp32c2"}
message from ('192.168.10.154', 60509)={"level":1,"mac":"80:65:99:ea:ff:00","now":229576,"cores":1,"target":"esp32s2"}
```

# HTTP Server Using Tornado
```
cd $HOME
sudo apt install python3-pip python3-setuptools
python3 -m pip install -U pip
python3 -m pip install -U wheel
python3 -m pip install tornado
cd esp-mesh-lite-examples/mesh_http_client
cd tornado
python3 main.py
```
Open your browser and put the Server's IP in the address bar.   
![Image](https://github.com/user-attachments/assets/53cae944-cac3-44ae-aac0-74440ecaa67f)

# HTTP Server Using Flask
```
cd $HOME
sudo apt install python3-pip python3-setuptools
python3 -m pip install -U pip
python3 -m pip install -U wheel
python3 -m pip install -U Werkzeug
python3 -m pip install flask
cd esp-mesh-lite-examples/mesh_http_client
cd flask
python3 main.py
```

![Image](https://github.com/user-attachments/assets/2db710f6-5a5b-4aa4-9b01-65784f6cd18a)
