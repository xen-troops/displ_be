add_subdirectory(protocols)
	
set(SOURCES
	Compositor.cpp
	Connector.cpp
	Display.cpp
	SharedBuffer.cpp
	SharedFile.cpp
	SharedMemory.cpp
	Shell.cpp
	ShellSurface.cpp
	Surface.cpp
)

if(WITH_DRM)
	list(APPEND SOURCES
		DrmBuffer.cpp
		WaylandDrm.cpp
	)
endif()

if(WITH_IVI_EXTENSION)
	list(APPEND SOURCES
		IviApplication.cpp
		IviSurface.cpp
	)
endif()

if(WITH_INPUT)
	list(APPEND SOURCES
		Seat.cpp
		SeatKeyboard.cpp
		SeatPointer.cpp
		SeatTouch.cpp
	)
endif()

add_library(display_wayland STATIC ${SOURCES})

if(WITH_DRM)
	target_link_libraries(display_wayland wayland_drm_protocol)
endif()

target_link_libraries(display_wayland ${WAYLAND_LIBRARIES})

if(WITH_IVI_EXTENSION)
	target_link_libraries(display_wayland
		${IVI_APP_LIBRARIES}
		${ILM_CTRL_LIBRARIES}
		${ILM_COMMON_LIBRARIES}
	)
endif()

target_link_libraries(display_wayland xenbe)