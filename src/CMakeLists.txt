include(AMXConfig)
include(AddSAMPPlugin)


set(SAMP_SDK_ROOT "${PROJECT_SOURCE_DIR}/libs/sdk")
find_package(SAMPSDK REQUIRED)

set(DCC_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/config_headers")

configure_file(
	"../discord-connector.inc.in"
	"discord-connector.inc"
	@ONLY
)

configure_file(
	"version.hpp.in"
	"${DCC_CONFIG_INCLUDE_DIR}/version.hpp"
	@ONLY
)


add_samp_plugin(discord-connector 
	${SAMPSDK_DIR}/amxplugin.cpp
	${SAMPSDK_DIR}/amxplugin2.cpp
	Bot.cpp
	Bot.hpp
	Channel.cpp
	Channel.hpp
	Error.hpp
	Guild.cpp
	Guild.hpp
	Logger.cpp
	Logger.hpp
	Message.cpp
	Message.hpp
	Network.cpp
	Network.hpp
	Singleton.hpp
	Http.cpp
	Http.hpp
	Callback.hpp
	PawnDispatcher.cpp
	PawnDispatcher.hpp
	Role.cpp
	Role.hpp
	SampConfigReader.cpp
	SampConfigReader.hpp
	User.cpp
	User.hpp
	WebSocket.cpp
	WebSocket.hpp
	main.cpp
	misc.hpp
	natives.cpp
	natives.hpp
	sdk.hpp
	types.hpp
	utils.hpp
	plugin.def
)

if (UNIX)
	target_sources(discord-connector PUBLIC ${SAMPSDK_DIR}/amx/getch.c)
endif()

set_property(TARGET discord-connector PROPERTY CXX_STANDARD 11)
set_property(TARGET discord-connector PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(discord-connector PRIVATE
	${OPENSSL_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
	${SAMPSDK_INCLUDE_DIR}
	${DCC_CONFIG_INCLUDE_DIR}
	${LOGCORE_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/libs/json
	${PROJECT_SOURCE_DIR}/libs/date/include
)

if(MSVC)
	target_compile_definitions(discord-connector PRIVATE
		BOOST_ALL_NO_LIB # disable auto-linking
		_CRT_SECURE_NO_WARNINGS
		NOMINMAX
		_WIN32_WINNT=0x0600
		_SCL_SECURE_NO_WARNINGS
	)
	target_link_libraries(discord-connector Crypt32.lib)

endif()

target_compile_definitions(discord-connector PRIVATE ONLY_C_LOCALE) # make the date library use the C locale only

target_link_libraries(discord-connector log-core fmt OpenSSL::SSL)

if(UNIX)
	if(NOT APPLE)
		target_link_libraries(discord-connector rt)
	endif()
	
	set_target_properties(discord-connector PROPERTIES
		INSTALL_RPATH "$ORIGIN/..:$ORIGIN/")
endif()

# increase warning levels
if (MSVC)
	target_compile_options(discord-connector PRIVATE
		/W4
		/WX
		/permissive-
	)

	# also enable multi-processor compilation
	target_compile_options(discord-connector PRIVATE
		/MP
	)
elseif(UNIX)
	target_compile_options(discord-connector PRIVATE
		-Wall
		-Wextra
		-pedantic
		-Werror
	)
endif()


install(TARGETS discord-connector DESTINATION "plugins/")
get_target_property(LOGCORE_LOCATION log-core LOCATION)
install(FILES "${LOGCORE_LOCATION}" DESTINATION "./")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/discord-connector.inc" DESTINATION "pawno/include/")
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "./")
if (WIN32 AND NOT OPENSSL_USE_STATIC_LIBS)
	file (GLOB _openssl_dlls  "${OPENSSL_ROOT_DIR}/bin/lib*.dll")
	foreach (_dll_file ${_openssl_dlls})
		install (FILES "${_dll_file}" DESTINATION "./")
	endforeach()
endif()


set(CPACK_PACKAGE_VERSION ${DCC_VERSION})
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
