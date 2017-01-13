cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "displ_be")

project(${PROJECT_NAME})

set(DRM_SOURCES
	src/drm/Connector.cpp
	src/drm/Device.cpp
	src/drm/Dumb.cpp
	src/drm/FrameBuffer.cpp
	src/drm/Modes.cpp
)

set(WL_SOURCES
	src/wayland/Compositor.cpp
	src/wayland/Connector.cpp
	src/wayland/Display.cpp
	src/wayland/IviApplication.cpp
	src/wayland/IviSurface.cpp
	src/wayland/SharedBuffer.cpp
	src/wayland/Seat.cpp
	src/wayland/SeatKeyboard.cpp
	src/wayland/SeatPointer.cpp
	src/wayland/SeatTouch.cpp
	src/wayland/SharedFile.cpp
	src/wayland/SharedMemory.cpp
	src/wayland/Shell.cpp
	src/wayland/ShellSurface.cpp
	src/wayland/Surface.cpp
)

set(WL_INPUT_SOURCES
	src/input/LibInput.cpp
	src/input/InputManager.cpp
)

set(BE_SOURCES
	src/Backend.cpp
	src/BuffersStorage.cpp
	src/DisplayBackend.cpp
	src/DisplayCommandHandler.cpp
	src/InputBackend.cpp
	src/InputHandlers.cpp
)

set(TEST_SOURCES
	src/Test.cpp
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

add_library(be_drm STATIC ${DRM_SOURCES})

add_library(be_wl STATIC ${WL_SOURCES})

add_library(be_wl_input STATIC ${WL_INPUT_SOURCES})

add_executable(${PROJECT_NAME} ${BE_SOURCES})

target_link_libraries(${PROJECT_NAME}
	be_wl_input
	be_drm
	be_wl
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
	be_drm
	be_wl
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
