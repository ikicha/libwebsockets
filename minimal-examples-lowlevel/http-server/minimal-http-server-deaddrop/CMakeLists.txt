project(lws-minimal-http-server-deaddrop C)
cmake_minimum_required(VERSION 3.10)
find_package(libwebsockets CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LWS_CMAKE_DIR})
include(CheckCSourceCompiles)
include(LwsCheckRequirements)

set(SAMP lws-minimal-http-server-deaddrop)
set(SRCS minimal-http-server-deaddrop.c)

# NOTE... if you are building this standalone, you must point LWS_PLUGINS_DIR
# to the lws plugins dir so it can pick up the plugin source.  Eg,
# cmake . -DLWS_PLUGINS_DIR=~/libwebsockets/plugins

set(requirements 1)
require_lws_config(LWS_ROLE_H1 1 requirements)
require_lws_config(LWS_ROLE_WS 1 requirements)
require_lws_config(LWS_WITH_SERVER 1 requirements)
require_lws_config(LWS_WITH_HTTP_BASIC_AUTH 1 requirements)
require_lws_config(LWS_WITH_FILE_OPS 1 requirements)

if (requirements AND UNIX)
	add_executable(${SAMP} ${SRCS})

	if (LWS_PLUGINS_DIR)
		include_directories(${LWS_PLUGINS_DIR})
	endif()

	if (websockets_shared)
		target_link_libraries(${SAMP} websockets_shared ${LIBWEBSOCKETS_DEP_LIBS})
		add_dependencies(${SAMP} websockets_shared)
	else()
		target_link_libraries(${SAMP} websockets ${LIBWEBSOCKETS_DEP_LIBS})
	endif()
endif()
