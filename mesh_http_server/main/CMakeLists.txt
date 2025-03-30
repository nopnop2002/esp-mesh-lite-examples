set(srcs "main.c")

if (CONFIG_MESH_ROOT)
	list(APPEND srcs "http_server.c")
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
