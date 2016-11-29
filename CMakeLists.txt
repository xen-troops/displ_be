cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "displ_be")

project(${PROJECT_NAME})

set(SOURCES
	src/drm/Connector.cpp
	src/drm/Device.cpp
	src/drm/Dumb.cpp
	src/drm/FrameBuffer.cpp
	src/drm/Modes.cpp
	src/wayland/Compositor.cpp
	src/wayland/Connector.cpp
	src/wayland/Display.cpp
	src/wayland/SharedBuffer.cpp
	src/wayland/SharedFile.cpp
	src/wayland/SharedMemory.cpp
	src/wayland/Shell.cpp
	src/wayland/ShellSurface.cpp
	src/wayland/Surface.cpp
	src/CommandHandler.cpp
	src/DisplayBackend.cpp
)

include_directories(
	src
	${XEN_INCLUDE_PATH}
	${XENBE_INCLUDE_PATH}
	${IF_INCLUDE_PATH}
	/usr/include/libdrm
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

link_directories(${XEN_LIB_PATH} ${XENBE_LIB_PATH})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
	xenbe
	xenctrl
	xenstore
	xenevtchn
	xengnttab
	drm
	wayland-client
	pthread
)

find_package(Doxygen)
if(DOXYGEN_FOUND)
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
endif(DOXYGEN_FOUND)
