cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project(discord-connector VERSION 0.2.4)

if (NOT DCC_VERSION) # allow version override
	set(DCC_VERSION ${PROJECT_VERSION})
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

if (MSVC)
	add_link_options(/INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
	# link runtime statically
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX)
	# hide non-exported symbols
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

	# link runtime statically
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	add_link_options(
		"-static-libgcc"
		"-static-libstdc++"
	)
endif()

if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
	set(OPENSSL_USE_STATIC_LIBS ON)
	set(OPENSSL_MSVC_STATIC_RT ON)
endif()
find_package(OpenSSL)
if (NOT OPENSSL_FOUND)
	set(OPENSSL_ROOT_DIR "${OPENSSL_ROOT_DIR}" CACHE PATH "Set the OpenSSL root folder.")
	mark_as_advanced(CLEAR OPENSSL_ROOT_DIR)
	message(FATAL_ERROR "Could NOT find OpenSSL; specify additional paths.")
endif()


find_package(Boost 1.69 QUIET)
if(NOT Boost_FOUND)
	set(BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Set the Boost root folder.")
	mark_as_advanced(CLEAR BOOST_ROOT)
	message(FATAL_ERROR "Could NOT find Boost; specify additional paths.")
endif()
mark_as_advanced(BOOST_ROOT Boost_DIR)


add_subdirectory(libs/fmt)
mark_as_advanced(FMT_TEST FMT_INSTALL FMT_PEDANTIC FMT_DOC FMT_USE_CPP11 FMT_WERROR)


find_package(log-core REQUIRED CONFIG)


add_subdirectory(src)
