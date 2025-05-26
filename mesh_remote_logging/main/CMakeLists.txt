set(srcs "main.c" "net_logging.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
