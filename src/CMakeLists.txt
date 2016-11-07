include(AMXConfig)
include(AddSAMPPlugin)


set(SAMP_SDK_ROOT "${PROJECT_SOURCE_DIR}/libs/sdk")
find_package(SAMPSDK REQUIRED)

set(DCC_PLUGIN_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/config_headers")
include_directories(
	${SAMPSDK_INCLUDE_DIR}
	${DCC_PLUGIN_CONFIG_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/libs/samp-log-core/include
	${PROJECT_SOURCE_DIR}/libs/samp-log-core/libs/cppformat
	${PROJECT_SOURCE_DIR}/libs/beast/include
	${PROJECT_SOURCE_DIR}/libs/json
)

configure_file(
	"../discord-connector.inc.in"
	"discord-connector.inc"
	@ONLY
)

configure_file(
	"version.hpp.in"
	"${DCC_PLUGIN_CONFIG_INCLUDE_DIR}/version.hpp"
	@ONLY
)


add_samp_plugin(discord-connector 
	${SAMPSDK_DIR}/amxplugin.cpp
	${SAMPSDK_DIR}/amxplugin2.cpp
	${SAMPSDK_DIR}/amx/getch.c
	CCallback.cpp
	CCallback.hpp
	CDispatcher.cpp
	CDispatcher.hpp
	CError.hpp
	CLog.cpp
	CLog.hpp
	CNetwork.cpp
	CNetwork.hpp
	CSingleton.hpp
	main.cpp
	misc.hpp
	natives.cpp
	natives.hpp
	sdk.hpp
	types.hpp
	plugin.def
)

target_compile_features(discord-connector PUBLIC 
	cxx_auto_type 
	cxx_decltype 
	cxx_default_function_template_args
	cxx_defaulted_functions 
	cxx_delegating_constructors
	cxx_deleted_functions
	cxx_enum_forward_declarations
	cxx_explicit_conversions
	cxx_extended_friend_declarations
	cxx_inheriting_constructors
	cxx_lambdas
	cxx_noexcept
	cxx_nonstatic_member_init
	cxx_nullptr
	cxx_range_for
	cxx_reference_qualified_functions
	cxx_right_angle_brackets
	cxx_rvalue_references
	cxx_strong_enums
	cxx_variadic_templates
)

add_dependencies(discord-connector log-core fmt)

if(MSVC)
	add_definitions(
		-D_CRT_SECURE_NO_WARNINGS 
		-DNOMINMAX 
		-D_ENABLE_ATOMIC_ALIGNMENT_FIX 
		-D_WIN32_WINNT=0x0600
		-D_SCL_SECURE_NO_WARNINGS)
endif()

target_link_libraries(discord-connector log-core fmt OpenSSL::SSL)

if(UNIX)
	if(NOT APPLE)
		target_link_libraries(discord-connector rt)
	endif()
	
	set_target_properties(discord-connector PROPERTIES
		INSTALL_RPATH "$ORIGIN/..:$ORIGIN/")
endif()


install(TARGETS discord-connector DESTINATION "plugins/")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/discord-connector.inc" DESTINATION "pawno/include/")
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "./")


set(CPACK_PACKAGE_VERSION ${DCC_PLUGIN_VERSION})
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
