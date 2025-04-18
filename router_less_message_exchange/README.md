# router_less_message_exchange
esp-mesh-lite allows for network configuration without using a WiFi router.   
An official sample is available [here](https://github.com/espressif/esp-mesh-lite/tree/master/examples/no_router).   
The official sample allows you to build a router-less mesh network, but it does not have the ability to exchange data between nodes.   
Therefore, we added a function for exchanging data between nodes.   
```
+----------+                 +----------+                 +----------+
|   root   |---(broadcast)-->|  leaf1   |---(broadcast)-->|  leaf11  |
|          |<---(to root)----|          |<---(to root)----|          |
+----------+                 +----------+                 +----------+
```

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
- At least 2 x ESP32 development boards   
	No WiFi router is required.

# Installation
```
git clone https://github.com/nopnop2002/esp-mesh-lite-examples
cd esp-mesh-lite-examples/no_router_message_exchange
idf.py menuconfig
idf.py flash
```

# Configuration   
One device must be configured as the root node and the other devices as leaf nodes.   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/8fe1e1f7-25ef-478f-995f-4edbc4877df3)

# Root node logging
The root node notifies the leaf nodes of the message sequence number and the MAC address of the root node.   
```
I (2951207) no_router: System information, channel: 11, layer: 1, self mac: a4:cf:12:05:c6:34, parent bssid: 00:00:00:00:00:00, parent rssi: -120, free heap: 201300
I (2951212) no_router: Child mac: 3c:71:bf:9d:bd:00
I (2951217) no_router: Child mac: 24:0a:c4:c5:46:fc
1: 2, 24:0a:c4:c5:46:fc, 192.168.5.3
2: 2, 3c:71:bf:9d:bd:00, 192.168.5.2
3: 1, a4:cf:12:05:c6:34, 0.0.0.0
[send to broadcast] number: 303, mac=a4:cf:12:05:c6:34
[recv from child] level: 2, number=35, mac: 3c:71:bf:9d:bd:00
[recv from child] level: 2, number=37, mac: 24:0a:c4:c5:46:fc
```


# Leaf node logging
The leaf node notifies the root node of the layer, message sequence number, and the MAC address of the leaf node.
```
I (311192) no_router: System information, channel: 11, layer: 2, self mac: 24:0a:c4:c5:46:fc, parent bssid: a4:cf:12:05:c6:35, parent rssi: -79, free heap: 205568
1: 1, a4:cf:12:05:c6:34, 0.0.0.0
2: 2, 3c:71:bf:9d:bd:00, 192.168.5.2
3: 2, 24:0a:c4:c5:46:fc, 192.168.5.3
[send to root] level: 2, number: 37, mac: 24:0a:c4:c5:46:fc
[recv from root] number: 303, mac: a4:cf:12:05:c6:34
```


# Reference
To communicate between nodes, use one of the following functions:
- esp_mesh_lite_send_msg   
	https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/include/esp_mesh_lite_core.h#L1032

- esp_mesh_lite_try_sending_msg   
	https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/include/esp_mesh_lite_core.h#L1049


Mesh-Lite supports the following intra-node communications:
- esp_mesh_lite_send_broadcast_msg_to_child()
- esp_mesh_lite_send_broadcast_msg_to_parent()
- esp_mesh_lite_send_msg_to_root()
- esp_mesh_lite_send_msg_to_parent()

The sent message will be received by a callback function.   
The callback function is defined using ```esp_mesh_lite_msg_action_list_register()```.   
The callback function definition follows this format:   
```
typedef struct esp_mesh_lite_msg_action {
    const char* type;         /**< The type of message sent */
    const char* rsp_type;     /**< The message type expected to be received. When a message of the expected type is received, stop retransmitting.
                                If set to NULL, it will be sent until the maximum number of retransmissions is reached. */
    msg_process_cb_t process; /**< The callback function when receiving the 'type' message. The cjson information in the type message can be processed in this cb. */
} esp_mesh_lite_msg_action_t;
```

This project defines the following receive callbacks:   
A message of type "broadcast" indicates that it will be notified to broadcast_process().   
A message of type "report_info_to_root" indicates that it will be notified to report_info_to_root_process().   

```
static const esp_mesh_lite_msg_action_t node_report_action[] = {
    {"broadcast", NULL, broadcast_process},

    /* Report information to the sibling node */
    {"report_info_to_sibling", NULL, report_info_to_sibling_process},

    /* Report information to the root node */
    {"report_info_to_root", "report_info_to_root_ack", report_info_to_root_process},
    {"report_info_to_root_ack", NULL, report_info_to_root_ack_process},

    /* Report information to the root node */
    {"report_info_to_parent", "report_info_to_parent_ack", report_info_to_parent_process},
    {"report_info_to_parent_ack", NULL, report_info_to_parent_ack_process},

    {NULL, NULL, NULL} /* Must be NULL terminated */
};
```

For more information, see [here](https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/include/esp_mesh_lite_core.h).   

