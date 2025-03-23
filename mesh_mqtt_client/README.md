# mesh_mqtt_client
MQTT client for esp-mesh-lite.

# Installation

```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/mesh_mqtt_client
idf.py menuconfig
idf.py flash
```

# Configuration   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/99635150-9cdc-4dc9-9b2f-bf0659a164f8)

## WiFi Setting
Set the information of your access point.
![Image](https://github.com/user-attachments/assets/9c68c775-4970-4a47-b15a-2fb96521060a)

## Broker Setting
Set the information of your MQTT broker.
![Image](https://github.com/user-attachments/assets/11906003-7812-4913-af2e-ace46d0e5241)

- Using TCP Port.   
 TCP Port uses the MQTT protocol.   

- Using SSL/TLS Port.   
 SSL/TLS Port uses the MQTTS protocol instead of the MQTT protocol.   

- Using WebSocket Port.   
 WebSocket Port uses the WS protocol instead of the MQTT protocol.   

- Using WebSocket Secure Port.   
 WebSocket Secure Port uses the WSS protocol instead of the MQTT protocol.   

__Note for using secure port.__   
The default MQTT server is ```broker.emqx.io```.   
If you use a different server, you will need to modify ```getpem.sh``` to run.   
```
chmod 777 getpem.sh
./getpem.sh
```

WebSocket/WebSocket Secure Port may differ depending on the broker used.   
If you use a different MQTT server than the default, you will need to change the port number from the default.   

### Specifying an MQTT Broker   
You can specify your MQTT broker in one of the following ways:   
- IP address   
 ```192.168.10.20```   
- mDNS host name   
 ```mqtt-broker.local```   
- Fully Qualified Domain Name   
 ```broker.emqx.io```

### Select MQTT Protocol   
This project supports MQTT Protocol V3.1.1/V5.   
![Image](https://github.com/user-attachments/assets/5115df83-ed55-4669-ae0e-7764e10dad7b)

### Enable Secure Option
Specifies the username and password if the server requires a password when connecting.   
[Here's](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-debian-10) how to install and secure the Mosquitto MQTT messaging broker on Debian 10.   
![Image](https://github.com/user-attachments/assets/63244a41-4805-457c-9dd1-9f36c95ad617)

# How to use
Run mosquitto_sub on the host side.   
MQTT topic has the following syntax:   
```/topic/mesh/{mesh_layer}```

```
$ sudo apt install moreutils
$ BROKER="broker.emqx.io"
$ TOPIC="/topic/mesh/#"
$ mosquitto_sub -h ${BROKER} -t ${TOPIC} -v | ts "%Y/%m/%d %H:%M:%S"
2025/03/23 08:52:30 /topic/mesh/2 data from 3c:71:bf:9d:bd:00
2025/03/23 08:52:30 /topic/mesh/1 data from a4:cf:12:05:c6:34
2025/03/23 08:52:31 /topic/mesh/2 data from 3c:71:bf:9d:bd:00
2025/03/23 08:52:31 /topic/mesh/1 data from a4:cf:12:05:c6:34
2025/03/23 08:52:32 /topic/mesh/2 data from 3c:71:bf:9d:bd:00
2025/03/23 08:52:32 /topic/mesh/1 data from a4:cf:12:05:c6:34
```

