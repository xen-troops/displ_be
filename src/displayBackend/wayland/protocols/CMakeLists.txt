find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

if(WITH_DRM)
	add_custom_command(
		OUTPUT  wayland-drm-client-protocol.h
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
				< ${CMAKE_CURRENT_LIST_DIR}/wayland-drm.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/wayland-drm-client-protocol.h
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/wayland-drm.xml
	)

	add_custom_command(
		OUTPUT  wayland-drm-protocol.c
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
				< ${CMAKE_CURRENT_LIST_DIR}/wayland-drm.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/wayland-drm-protocol.c
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/wayland-drm.xml
	)

	add_library(wayland_drm_protocol STATIC
		${CMAKE_CURRENT_BINARY_DIR}/wayland-drm-client-protocol.h
		${CMAKE_CURRENT_BINARY_DIR}/wayland-drm-protocol.c
	)
endif()