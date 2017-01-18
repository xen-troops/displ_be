cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "displ_be")

project(${PROJECT_NAME})

set(DRM_SOURCES
	src/displayBackend/drm/Connector.cpp
	src/displayBackend/drm/Display.cpp
	src/displayBackend/drm/Dumb.cpp
	src/displayBackend/drm/DumbZeroCopy.cpp
	src/displayBackend/drm/FrameBuffer.cpp
	src/displayBackend/drm/Modes.cpp
)

set(WL_SOURCES
	src/displayBackend/wayland/wayland-drm/wayland-drm-protocol.c
	src/displayBackend/wayland/Compositor.cpp
	src/displayBackend/wayland/Connector.cpp
	src/displayBackend/wayland/DrmBuffer.cpp
	src/displayBackend/wayland/Display.cpp
	src/displayBackend/wayland/IviApplication.cpp
	src/displayBackend/wayland/IviSurface.cpp
	src/displayBackend/wayland/SharedBuffer.cpp
	src/displayBackend/wayland/Seat.cpp
	src/displayBackend/wayland/SeatKeyboard.cpp
	src/displayBackend/wayland/SeatPointer.cpp
	src/displayBackend/wayland/SeatTouch.cpp
	src/displayBackend/wayland/SharedFile.cpp
	src/displayBackend/wayland/SharedMemory.cpp
	src/displayBackend/wayland/Shell.cpp
	src/displayBackend/wayland/ShellSurface.cpp
	src/displayBackend/wayland/Surface.cpp
	src/displayBackend/wayland/WaylandDrm.cpp
)

set(WL_INPUT_SOURCES
	src/inputBackend/input/LibInput.cpp
	src/inputBackend/input/InputManager.cpp
)

set(BE_SOURCES
	src/main.cpp
	src/displayBackend/BuffersStorage.cpp
	src/displayBackend/DisplayBackend.cpp
	src/displayBackend/DisplayCommandHandler.cpp
	src/inputBackend/InputBackend.cpp
	src/inputBackend/InputHandlers.cpp
)

set(TEST_SOURCES
	src/test.cpp
)

include_directories(
	src
	src/displayBackend
	src/inputBackend
	${XEN_INCLUDE_PATH}
	${XENBE_INCLUDE_PATH}
	${IF_INCLUDE_PATH}
	${DRMMAP_INCLUDE_PATH}
	/usr/include/libdrm
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

link_directories(${XEN_LIB_PATH} ${XENBE_LIB_PATH})

add_library(be_drm STATIC ${DRM_SOURCES})

add_library(be_wl STATIC ${WL_SOURCES})

add_library(be_wl_input STATIC ${WL_INPUT_SOURCES})

add_executable(${PROJECT_NAME} ${BE_SOURCES})

target_link_libraries(${PROJECT_NAME}
	be_wl_input
	be_wl
	be_drm
	xenbe
	xenctrl
	xenstore
	xenevtchn
	xengnttab
	drm
	wayland-client
	ilmClient
	pthread
)

add_executable(displ_test ${TEST_SOURCES})

target_link_libraries(displ_test
	be_wl_input
	be_wl
	be_drm
	xenbe
	xenctrl
	xenstore
	xenevtchn
	xengnttab
	drm
	wayland-client
	ilmClient
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
