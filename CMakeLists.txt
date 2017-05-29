cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "displ_be")

project(${PROJECT_NAME})

################################################################################
# Options
################################################################################

OPTION(WITH_DRM "build with DRM backend" ON)
OPTION(WITH_ZCOPY "build with zero copy support" ON)
OPTION(WITH_WAYLAND "build with wayland backend" ON)
OPTION(WITH_IVI_EXTENSION "build with wayland IVI Extension" ON)
OPTION(WITH_INPUT "build with input backend" ON)
OPTION(WITH_DOC "build with documenation" OFF)

message(STATUS)
message(STATUS "${PROJECT_NAME} Configuration:")
message(STATUS "CMAKE_BUILD_TYPE              = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX          = ${CMAKE_INSTALL_PREFIX}")
message(STATUS)
message(STATUS "WITH_DOC                      = ${WITH_DOC}")
message(STATUS "WITH_DRM                      = ${WITH_DRM}")
message(STATUS "WITH_ZCOPY                    = ${WITH_ZCOPY}")
message(STATUS "WITH_WAYLAND                  = ${WITH_WAYLAND}")
message(STATUS "WITH_IVI_EXTENSION            = ${WITH_IVI_EXTENSION}")
message(STATUS "WITH_INPUT                    = ${WITH_INPUT}")
message(STATUS)
message(STATUS "XEN_INCLUDE_PATH              = ${XEN_INCLUDE_PATH}")
message(STATUS "XENBE_INCLUDE_PATH            = ${XENBE_INCLUDE_PATH}")
message(STATUS "IF_INCLUDE_PATH               = ${IF_INCLUDE_PATH}")
message(STATUS "DRMZCOPY_INCLUDE_PATH         = ${DRMZCOPY_INCLUDE_PATH}")
message(STATUS)
message(STATUS "XEN_LIB_PATH                  = ${XEN_LIB_PATH}")
message(STATUS "XENBE_LIB_PATH                = ${XENBE_LIB_PATH}")
message(STATUS)

if(NOT WITH_WAYLAND AND NOT WITH_DRM AND NOT WITH_INPUT)
	message(FATAL_ERROR "At least one backend should be specified: WITH_DRM, WITH_WAYLAND, WITH_INPUT")
endif()

################################################################################
# Compiler flags
################################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

################################################################################
# Definitions
################################################################################

if(WITH_DRM OR WITH_WAYLAND)
	add_definitions(-DWITH_DISPLAY)
endif()

if(WITH_DRM)
	add_definitions(-DWITH_DRM)
	if(WITH_ZCOPY)
		add_definitions(-DWITH_ZCOPY)
	endif()
endif()

if(WITH_WAYLAND)
	add_definitions(-DWITH_WAYLAND)
	if(WITH_IVI_EXTENSION)
		add_definitions(-DWITH_IVI_EXTENSION)
	endif()
endif()

if(WITH_INPUT)
	add_definitions(-DWITH_INPUT)
endif()

################################################################################
# Includes
################################################################################

include_directories(
	${XEN_INCLUDE_PATH}
	${XENBE_INCLUDE_PATH}
	${DRMZCOPY_INCLUDE_PATH}
	.
)

################################################################################
# Link directories
################################################################################

link_directories(
	${XEN_LIB_PATH}
	${XENBE_LIB_PATH}
)

################################################################################
# Sources
################################################################################

add_subdirectory(src)

################################################################################
# Docs
################################################################################

find_package(Doxygen)

if(WITH_DOC AND NOT DOXYGEN_FOUND)
	message(FATAL_ERROR "Doxygen not found. Required to generate docs.")
else()
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg
		${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		@ONLY
	)
	
	add_custom_target(
		doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif()
