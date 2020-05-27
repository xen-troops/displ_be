find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

if(WITH_ZCOPY)
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

	add_custom_command(
		OUTPUT  wayland-kms-client-protocol.h
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
				< ${CMAKE_CURRENT_LIST_DIR}/wayland-kms.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/wayland-kms-client-protocol.h
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/wayland-kms.xml
	)

	add_custom_command(
		OUTPUT  wayland-kms-protocol.c
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
				< ${CMAKE_CURRENT_LIST_DIR}/wayland-kms.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/wayland-kms-protocol.c
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/wayland-kms.xml
	)

	add_library(wayland_kms_protocol STATIC
		${CMAKE_CURRENT_BINARY_DIR}/wayland-kms-client-protocol.h
		${CMAKE_CURRENT_BINARY_DIR}/wayland-kms-protocol.c
	)

	add_custom_command(
		OUTPUT  linux-dmabuf-unstable-v1-client-protocol.h
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
				< ${CMAKE_CURRENT_LIST_DIR}/linux-dmabuf-unstable-v1.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1-client-protocol.h
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/linux-dmabuf-unstable-v1.xml
	)

	add_custom_command(
		OUTPUT  linux-dmabuf-unstable-v1-protocol.c
		COMMAND ${WAYLAND_SCANNER_EXECUTABLE} code
				< ${CMAKE_CURRENT_LIST_DIR}/linux-dmabuf-unstable-v1.xml
				> ${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1-protocol.c
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/linux-dmabuf-unstable-v1.xml
	)

	add_library(linux_dmabuf_unstable_v1_protocol STATIC
		${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1-client-protocol.h
		${CMAKE_CURRENT_BINARY_DIR}/linux-dmabuf-unstable-v1-protocol.c
	)

endif()