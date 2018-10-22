cmake_minimum_required(VERSION 3.1)
project(discord-connector)

set(DCC_PLUGIN_VERSION "0.2.4")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

if(UNIX)
	#force 32bit compilation and hide non-exported symbols
	set(CMAKE_CXX_FLAGS "-m32 -fvisibility=hidden")
	set(CMAKE_C_FLAGS "-m32 -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
endif()


if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
	set(OPENSSL_USE_STATIC_LIBS ON)
endif()
find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
	set(OPENSSL_ROOT_DIR "${OPENSSL_ROOT_DIR}" CACHE PATH "Set the OpenSSL root folder.")
	mark_as_advanced(CLEAR OPENSSL_ROOT_DIR)
	message(FATAL_ERROR "Could NOT find OpenSSL; specify additional paths.")
endif()


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.66 COMPONENTS system)

if(NOT Boost_FOUND)
	set(BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Set the Boost root folder.")
	set(BOOST_LIBRARYDIR "${BOOST_LIBRARYDIR}" CACHE PATH "Set the Boost library folder.")
	mark_as_advanced(CLEAR BOOST_ROOT BOOST_LIBRARYDIR)
	message(FATAL_ERROR "Could NOT find Boost; specify additional paths.")
endif()


add_subdirectory(libs/fmt)
mark_as_advanced(FMT_DOC FMT_INSTALL FMT_PEDANTIC FMT_TEST FMT_USE_CPP11 FMT_WERROR)


find_package(log-core REQUIRED CONFIG)


add_subdirectory(src)
