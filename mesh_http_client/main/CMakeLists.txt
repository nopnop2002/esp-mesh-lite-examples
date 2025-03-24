set(srcs "main.c" "http_client.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
