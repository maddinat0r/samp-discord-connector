cmake_minimum_required(VERSION 3.1)
project(discord-connector)

set(DCC_PLUGIN_VERSION "0.1")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

if(UNIX)
	#force 32bit compilation and hide non-exported symbols
	set(CMAKE_CXX_FLAGS "-m32 -fvisibility=hidden")
	set(CMAKE_C_FLAGS "-m32 -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
endif()


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58 QUIET COMPONENTS system)

if(NOT Boost_FOUND)
	set(BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Set the Boost root folder.")
	set(BOOST_LIBRARYDIR "${BOOST_LIBRARYDIR}" CACHE PATH "Set the Boost library folder.")
	mark_as_advanced(CLEAR BOOST_ROOT BOOST_LIBRARYDIR)
	message(FATAL_ERROR "Could NOT find Boost; specify additional paths.")
endif()
mark_as_advanced(BOOST_ROOT BOOST_LIBRARYDIR Boost_DIR)

include_directories("${Boost_INCLUDE_DIR}")

if(MSVC)
	#needed for Boost's auto-link feature
	link_directories("${Boost_LIBRARY_DIRS}")
endif()


option(LOGCORE_INSTALL_DEV OFF)
mark_as_advanced(LOGCORE_INSTALL_DEV)
add_subdirectory(libs/samp-log-core)


add_subdirectory(src)
