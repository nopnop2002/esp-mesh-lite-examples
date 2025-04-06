set(srcs "main.c")

if (CONFIG_MESH_ROOT)
	list(APPEND srcs "json_server.c")
	list(APPEND srcs "node_manager.c")
	list(APPEND srcs "web_server.c")
	list(APPEND srcs "web_client.c")
	idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "."
 	   EMBED_FILES "../html/error.html"
                "../html/favicon.ico"
                "../html/main.js"
                "../html/root.html"
                "../html/main.css"
                "../html/libs/app/reset.css"
                "../html/libs/app/app.css"
                "../html/libs/jsonTree/jsonTree.css"
                "../html/libs/jsonTree/jsonTree.js"
                "../html/libs/jsonTree/icons.svg")
else()
	idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")
endif()

