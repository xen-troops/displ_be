################################################################################
# Check packages
################################################################################

include(FindPkgConfig)

if(WITH_SYSTEMD)
	pkg_check_modules (SYSTEMD REQUIRED libsystemd)
endif()

pkg_check_modules (DRM REQUIRED libdrm)

if(WITH_WAYLAND)
	pkg_check_modules (WAYLAND REQUIRED wayland-client)
endif()

if(WITH_IVI_EXTENSION AND WITH_WAYLAND)
	pkg_check_modules (IVI_APP ivi-application)
	if(NOT IVI_APP_FOUND)
		set(IVI_APP_LIBRARIES ivi-application)
	endif()
endif()

################################################################################
# Includes
################################################################################

include_directories(
	.
	include_directories(${DRM_INCLUDE_DIRS})
)

if (WITH_WAYLAND)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/displayBackend/wayland/protocols)
endif()

if(WITH_DRM OR WITH_WAYLAND)
	include_directories(displayBackend)
endif()

if(WITH_INPUT)
	include_directories(inputBackend)
endif()

################################################################################
# Sources
################################################################################

if(WITH_DRM OR WITH_WAYLAND)
	add_subdirectory(displayBackend)
endif()

if(WITH_INPUT)
	add_subdirectory(inputBackend)
endif()

set(SOURCES
	main.cpp
)

if(WITH_MOCKBELIB)
	list(APPEND SOURCES
		MockBackend.cpp
	)
endif()

################################################################################
# Targets
################################################################################

add_executable(${PROJECT_NAME} ${SOURCES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

################################################################################
# Libraries
################################################################################

if(WITH_INPUT AND (WITH_DMR OR WITH_WAYLAND))
	add_dependencies(input display)
endif()

if(WITH_DRM OR WITH_WAYLAND)
	target_link_libraries(${PROJECT_NAME} display)
endif()

if(WITH_INPUT)
	target_link_libraries(${PROJECT_NAME} input)
endif()

set(XENBE_LIB xenbe)

if(WITH_MOCKBELIB)
	set(XENBE_LIB xenbemock)
endif()

target_link_libraries(${PROJECT_NAME}
	${XENBE_LIB}
	${SYSTEMD_LIBRARIES}
	pthread
)
