cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "displ_be")

project(${PROJECT_NAME})

################################################################################
# Options
################################################################################

OPTION(WITH_DRM "build with DRM backend" ON)
OPTION(WITH_ZCOPY "build with zero copy support" ON)
OPTION(WITH_DRM_ZCOPY "enable drm zero-copy support" ON)
OPTION(WITH_KMS_ZCOPY "enable kms zero-copy support" ON)
OPTION(WITH_DMABUF_ZCOPY "enable dmabuf zero-copy support" ON)
OPTION(WITH_WAYLAND "build with wayland backend" ON)
OPTION(WITH_WAYLAND_PRESENTATION_API "deliver callbacks in a more synchronous way using presentation time protocol" OFF)
OPTION(WITH_IVI_EXTENSION "build with wayland IVI Extension" ON)
OPTION(WITH_INPUT "build with input backend" ON)
OPTION(WITH_MOCKBELIB "build with mock backend lib" OFF)
OPTION(WITH_DOC "build with documenation" OFF)
OPTION(IGNORE_MODIFIER_VALUES "disable pixel format modifiers check (dangerous)" OFF)

message(STATUS)
message(STATUS "${PROJECT_NAME} Configuration:")
message(STATUS "CMAKE_BUILD_TYPE              = ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_INSTALL_PREFIX          = ${CMAKE_INSTALL_PREFIX}")
message(STATUS)
message(STATUS "WITH_DOC                      = ${WITH_DOC}")
message(STATUS "WITH_DRM                      = ${WITH_DRM}")
message(STATUS "WITH_ZCOPY                    = ${WITH_ZCOPY}")
message(STATUS "WITH_DRM_ZCOPY                = ${WITH_DRM_ZCOPY}")
message(STATUS "WITH_KMS_ZCOPY                = ${WITH_KMS_ZCOPY}")
message(STATUS "WITH_DMABUF_ZCOPY             = ${WITH_DMABUF_ZCOPY}")
message(STATUS "WITH_WAYLAND                  = ${WITH_WAYLAND}")
message(STATUS "WITH_WAYLAND_PRESENTATION_API = ${WITH_WAYLAND_PRESENTATION_API}")
message(STATUS "WITH_IVI_EXTENSION            = ${WITH_IVI_EXTENSION}")
message(STATUS "WITH_INPUT                    = ${WITH_INPUT}")
message(STATUS)
message(STATUS "WITH_MOCKBELIB                = ${WITH_MOCKBELIB}")
message(STATUS)
message(STATUS "IGNORE_MODIFIER_VALUES        = ${IGNORE_MODIFIER_VALUES}")
message(STATUS)
message(STATUS "XEN_INCLUDE_PATH              = ${XEN_INCLUDE_PATH}")
message(STATUS "XENBE_INCLUDE_PATH            = ${XENBE_INCLUDE_PATH}")
message(STATUS "IF_INCLUDE_PATH               = ${IF_INCLUDE_PATH}")
if(WITH_ZCOPY)
message(STATUS "DRMZCOPY_INCLUDE_PATH         = ${DRMZCOPY_INCLUDE_PATH}")
endif()
if(WITH_MOCKBELIB)
message(STATUS "XENBEMOCK_INCLUDE_PATH        = ${XENBEMOCK_INCLUDE_PATH}")
endif()
message(STATUS)
message(STATUS "XEN_LIB_PATH                  = ${XEN_LIB_PATH}")
message(STATUS "XENBE_LIB_PATH                = ${XENBE_LIB_PATH}")
if(WITH_MOCKBELIB)
message(STATUS "XENBEMOCK_LIB_PATH            = ${XENBEMOCK_LIB_PATH}")
endif()
message(STATUS)

if(NOT WITH_WAYLAND AND NOT WITH_DRM AND NOT WITH_INPUT)
	message(FATAL_ERROR "At least one backend should be specified: WITH_DRM, WITH_WAYLAND, WITH_INPUT")
endif()

if(NOT WITH_DRM AND WITH_ZCOPY)
	message(FATAL_ERROR "Can't enable zero copy without DRM.")
endif()

if (WITH_ZCOPY AND NOT WITH_DRM_ZCOPY AND NOT WITH_KMS_ZCOPY AND NOT WITH_DMABUF_ZCOPY)
    message(FATAL_ERROR "At least one zero-copy function should be enabled")
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
		if(WITH_DRM_ZCOPY)
			add_definitions(-DWITH_DRM_ZCOPY)
		endif()
		if(WITH_KMS_ZCOPY)
			add_definitions(-DWITH_KMS_ZCOPY)
		endif()
		if(WITH_DMABUF_ZCOPY)
			add_definitions(-DWITH_DMABUF_ZCOPY)
		endif()
	endif()
endif()

if(WITH_WAYLAND)
	add_definitions(-DWITH_WAYLAND)
	if(WITH_IVI_EXTENSION)
		add_definitions(-DWITH_IVI_EXTENSION)
	endif()
	if(WITH_WAYLAND_PRESENTATION_API)
		add_definitions(-DWITH_WAYLAND_PRESENTATION_API)
	endif()
endif()

if(WITH_INPUT)
	add_definitions(-DWITH_INPUT)
endif()

if(WITH_MOCKBELIB)
	add_definitions(-DWITH_MOCKBELIB)
endif()

if(IGNORE_MODIFIER_VALUES)
	add_definitions(-DIGNORE_MODIFIER_VALUES)
endif()

################################################################################
# Includes
################################################################################

include_directories(
	${XEN_INCLUDE_PATH}
	${XENBE_INCLUDE_PATH}
	${XENBEMOCK_INCLUDE_PATH}
	${DRMZCOPY_INCLUDE_PATH}
	${CMAKE_CURRENT_BINARY_DIR}
	.
)

################################################################################
# Link directories
################################################################################

link_directories(
	${XEN_LIB_PATH}
	${XENBE_LIB_PATH}
	${XENBEMOCK_LIB_PATH}
)

################################################################################
# Sources
################################################################################

add_subdirectory(src)

################################################################################
# Versioning
################################################################################

find_package(Git)

if (NOT GIT_FOUND)
	set(GIT_EXECUTABLE git)
endif()

add_custom_target(Version
	COMMAND ${CMAKE_COMMAND}
	-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
	-D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Version.hpp.in
	-D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/Version.hpp
	-D GIT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
	-P ${CMAKE_SOURCE_DIR}/generateVersion.cmake
)

add_dependencies(${PROJECT_NAME} Version)

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
