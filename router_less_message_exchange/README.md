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
cd esp-mesh-lite-examples/router_less_message_exchange
idf.py menuconfig
idf.py flash
```

# Configuration   
One device must be configured as the root node and the other devices as leaf nodes.   
![Image](https://github.com/user-attachments/assets/28ee4b1b-541a-4bc0-9d20-4c70e0e60452)
![Image](https://github.com/user-attachments/assets/9f7c6490-0865-43f5-ac56-dae0bce6115d)

You can select JSON format or RAW format.   
![Image](https://github.com/user-attachments/assets/2cfddadc-cba8-4d92-8ce5-231ef553ef06)

You can enable AES encryption.   
Communication between nodes within a Mesh network can be encrypted with AES128 using esp_mesh_lite_aes_set_key.   
![Image](https://github.com/user-attachments/assets/87e9cd01-8319-4fc6-9a50-d703e0d41a16)


# JSON FORMAT   
To send data to other nodes, use the following function:   
```esp_mesh_lite_send_msg(ESP_MESH_LITE_JSON_MSG, &config)```   
The function pointer for sending a message is one of the following:   
- esp_mesh_lite_send_broadcast_msg_to_child()   
	All child nodes receive the message.   
	However, the grandchild nodes do not receive the message.   
	The child node's receive handler must forward the message to further child nodes.   
- esp_mesh_lite_send_broadcast_msg_to_parent()   
	The parent node's application layer callback will NOT receive the message,   
	but other child nodes under the same parent (=sibling node) will receive it.   
- esp_mesh_lite_send_msg_to_root()   
	Only the root node will receive the message.   
- esp_mesh_lite_send_msg_to_parent()   
	Only the parent node will receive the message.   
	The sender's direct parent can process the message in its application layer callback.   

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
A message of type "json_id_broadcast" indicates that it will be notified to json_broadcast_handler().   
Whe json_broadcast_handler() receives data, there is no response.   
```
+----------+                         +----------+
|   root   |---(json_id_broadcast)-->|   leaf   |
|          |                         |          |
+----------+                         +----------+
```


A message of type "json_id_to_root" indicates that it will be notified to json_to_root_handler().   
When json_to_root_handler() receives data, it responds with "json_id_to_root_ack".   
A message of type "json_id_to_root_ack" indicates that it will be notified to json_to_root_ack_handler().
```
+----------+                         +----------+
|   root   |<---(json_id_to_root)----|   leaf   |
|          |--(json_id_to_root_ack)->|          |
+----------+                         +----------+
```


```
static const esp_mesh_lite_msg_action_t json_msgs_action[] = {
    /* Send JSON to the all node */
    {"json_id_broadcast", NULL, json_broadcast_handler},

    /* Send JSON to the sibling node */
    {"json_id_to_sibling", NULL, json_to_sibling_handler},

    /* Send JSON to the root node */
    {"json_id_to_root", "json_id_to_root_ack", json_to_root_handler},
    {"json_id_to_root_ack", NULL, json_to_root_ack_handler},

    /* Send JSON to the parent node */
    {"json_id_to_parent", "json_id_to_parent_ack", json_to_parent_handler},
    {"json_id_to_parent_ack", NULL, json_to_parent_ack_handler},

    {NULL, NULL, NULL} /* Must be NULL terminated */
};
```


# RAW FORMAT   
To send data to other nodes, use the following function:   
```esp_mesh_lite_send_msg(ESP_MESH_LITE_RAW_MSG, &config)```   
The function pointer for sending a message is one of the following:   
- esp_mesh_lite_send_broadcast_raw_msg_to_child()   
- esp_mesh_lite_send_broadcast_raw_msg_to_parent()   
- esp_mesh_lite_send_raw_msg_to_root()   
- esp_mesh_lite_send_raw_msg_to_parent()   

The sent message will be received by a callback function.   
The callback function is defined using ```esp_mesh_lite_raw_msg_action_list_register()```.   
The callback function definition follows this format:   
```
typedef struct esp_mesh_lite_raw_msg_action {
    uint32_t msg_id;                  /**< The ID of the raw message sent */
    uint32_t resp_msg_id;             /**< The ID of the response message expected to be received. When a message with the expected ID is received, stop retransmitting.
                                       If set to 0, the message will be sent until the maximum number of retransmissions is reached. */
    raw_msg_process_cb_t raw_process; /**< The callback function when receiving the raw message. The raw message data can be processed in this callback. */
} esp_mesh_lite_raw_msg_action_t;
```

This project defines the following receive callbacks:   

```
#define RAW_MSG_ID_TO_ROOT 1
#define RAW_MSG_ID_TO_SIBLING 2
#define RAW_MSG_ID_TO_ROOT_RESP 3
#define RAW_MSG_ID_TO_PARENT 4
#define RAW_MSG_ID_TO_PARENT_RESP 5
#define RAW_MSG_ID_BROADCAST 6

static const esp_mesh_lite_raw_msg_action_t raw_msgs_action[] = {
    /* Send RAW to the all node */
    {RAW_MSG_ID_BROADCAST, 0, raw_broadcast_handler},

    /* Send RAW to the sibling node */
    {RAW_MSG_ID_TO_SIBLING, 0, raw_to_sibling_handler},

    /* Send RAW to the root node */
    {RAW_MSG_ID_TO_ROOT, RAW_MSG_ID_TO_ROOT_RESP, raw_to_root_handler},
    {RAW_MSG_ID_TO_ROOT_RESP, 0, raw_to_root_resp_handler},

    /* Send RAW to the parent node */
    {RAW_MSG_ID_TO_PARENT, RAW_MSG_ID_TO_PARENT_RESP, raw_to_parent_handler},
    {RAW_MSG_ID_TO_PARENT_RESP, 0, raw_to_parent_resp_handler},

    {0, 0, NULL} /* Must be NULL terminated */
};
```

# Loss of a leaf node   
If communication from a leaf node to the root node is lost for a certain period of time, the root node will determine that the leaf node is lost.   
This is the logging of the root node at that time.   
```
W (4296140) wifi:inactive timer: now=713bb last_rx_time=ee0e5bf3 diff=499c8, aid[2]3c:71:bf:9d:bd:00 leave
I (4296164) wifi:station: 3c:71:bf:9d:bd:00 leave, AID = 2, reason = 4, bss_flags is 753779, bss:0x3ffd22dc
I (4296165) wifi:new:<11,2>, old:<11,2>, ap:<11,2>, sta:<0,0>, prof:11, snd_ch_cfg:0x0
E (4296171) bridge_wifi: STA Disconnect to the AP
W (4297880) wifi:inactive timer: now=21a180 last_rx_time=ee2ea74b diff=49850, aid[1]c8:c9:a3:cf:10:c4 leave
I (4297905) wifi:station: c8:c9:a3:cf:10:c4 leave, AID = 1, reason = 4, bss_flags is 753779, bss:0x3ffba868
I (4297906) wifi:new:<11,0>, old:<11,2>, ap:<11,2>, sta:<0,0>, prof:11, snd_ch_cfg:0x0
E (4297913) bridge_wifi: STA Disconnect to the AP
```

# More information   
For more information, see [here](https://github.com/espressif/esp-mesh-lite/blob/master/components/mesh_lite/include/esp_mesh_lite_core.h).   

