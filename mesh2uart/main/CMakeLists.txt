set(srcs "main.c")

if (CONFIG_MESH_ROOT)
	if (CONFIG_UART_INTERFACE)
		list(APPEND srcs "uart.c")
	elseif (CONFIG_USB_INTERFACE)
		list(APPEND srcs "tusb.c")
	endif()
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
