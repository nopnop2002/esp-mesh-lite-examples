set(srcs "main.c" "ws_client.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
