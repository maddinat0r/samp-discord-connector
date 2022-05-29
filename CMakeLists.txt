cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project(discord-connector VERSION 0.3.6)

if (NOT DCC_VERSION) # allow version override
	set(DCC_VERSION ${PROJECT_VERSION})
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


if(MSVC)
	set(CMAKE_BUILD_TYPE "" CACHE STRING "" FORCE)
endif()

if(UNIX)
	set(
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifact/"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifact/"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/artifact/"
	)
	SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")
endif()

# -- CONAN
include(libs/cmake-conan/conan.cmake)
function(add_conan_lib pkg_name conan_pkg)
	conan_check(REQUIRED)

	conan_cmake_run(
		REQUIRES ${conan_pkg}
		ARCH x86
		BASIC_SETUP CMAKE_TARGETS
		BUILD missing
	)

	set_target_properties(${pkg_name} PROPERTIES IMPORTED_GLOBAL TRUE)
endfunction()

if (MSVC)
	# link runtime statically
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
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
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()

if(NOT DEFINED OPENSSL_USE_STATIC_LIBS)
	set(OPENSSL_USE_STATIC_LIBS ON)
	set(OPENSSL_MSVC_STATIC_RT ON)
endif()

if(USING_BOOST_NO_CONAN)
	set(Boost_USE_STATIC_LIBS ON)
	set(Boost_USE_DEBUG_LIBS ON)
	set(Boost_USE_RELEASE_LIBS ON)
	set(Boost_USE_MULTITHREADED ON)
	set(Boost_USE_STATIC_RUNTIME OFF)
	find_package(Boost 1.69 QUIET)
	if(NOT Boost_FOUND)
		set(BOOST_ROOT "${BOOST_ROOT}" CACHE PATH "Set the Boost root folder.")
		mark_as_advanced(CLEAR BOOST_ROOT)
		message(FATAL_ERROR "Could NOT find Boost; specify additional paths.")
	endif()
	mark_as_advanced(BOOST_ROOT Boost_DIR)

	set(Boost_INCLUDE_DIR "${Boost_INCLUDE_DIR}" PARENT_SCOPE)
else()
	conan_check(REQUIRED)
	conan_cmake_run(
		REQUIRES boost/1.73.0
		ARCH x86
		BASIC_SETUP CMAKE_TARGETS
		BUILD missing
		OPTIONS boost:zlib=False
	)
	set_target_properties(CONAN_PKG::boost PROPERTIES IMPORTED_GLOBAL TRUE)
endif()

set(CONAN_HAS_FMT TRUE CACHE INTERNAL FALSE)
add_conan_lib(CONAN_PKG::fmt fmt/7.1.3)

if(USING_OPENSSL_NO_CONAN)
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
else()
	add_conan_lib(CONAN_PKG::openssl openssl/1.1.1k)
endif()

add_subdirectory(libs/log-core)
add_subdirectory(src)
