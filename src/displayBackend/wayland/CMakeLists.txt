add_subdirectory(protocols)
	
set(SOURCES
	Compositor.cpp
	Connector.cpp
	Display.cpp
	FrameBuffer.cpp
	SharedFile.cpp
	SharedMemory.cpp
	Shell.cpp
	ShellSurface.cpp
	Surface.cpp
	SurfaceManager.cpp
)

if(WITH_DRM AND WITH_ZCOPY)
	list(APPEND SOURCES
		WaylandZCopy.cpp
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

target_link_libraries(display_wayland
	xdg_shell_protocol
)

if(WITH_ZCOPY)
	target_link_libraries(display_wayland
		wayland_drm_protocol
		wayland_kms_protocol
		linux_dmabuf_unstable_v1_protocol
	)
endif()

target_link_libraries(display_wayland ${WAYLAND_LIBRARIES})

if(WITH_IVI_EXTENSION)
	target_link_libraries(display_wayland
		${IVI_APP_LIBRARIES}
	)
endif()

target_link_libraries(display_wayland xenbe display_common)