################################################################################
# Check packages
################################################################################

include(FindPkgConfig)

pkg_check_modules (CONFIG REQUIRED libconfig++)
if (!CONFIG_FOUND)
	message(FATAL_ERROR "No libconfig++ found")
endif()

if(WITH_DRM)
	pkg_check_modules (DRM REQUIRED libdrm)
	if (!DRM_FOUND)
		message(FATAL_ERROR "No libdrm found")
	endif()
endif()

if(WITH_WAYLAND)
	pkg_check_modules (WAYLAND REQUIRED wayland-client)
	if(!WAYLAND_FOUND)
		message(FATAL_ERROR "No wayland-client found")
	endif()
endif()

################################################################################
# Includes
################################################################################

include_directories(
	.
	${CONFIG_INCLUDE_PATH}
)

if(WITH_DRM)
	include_directories(${DRM_INCLUDE_DIRS})
endif()

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
	Config.cpp
)

################################################################################
# Targets
################################################################################

add_executable(${PROJECT_NAME} ${SOURCES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

################################################################################
# Libraries
################################################################################

if(WITH_INPUT)
	add_dependencies (input display)
endif()

if(WITH_DRM OR WITH_WAYLAND)
	target_link_libraries(${PROJECT_NAME} display)
endif()

if(WITH_INPUT)
	target_link_libraries(${PROJECT_NAME} input)
endif()

target_link_libraries(${PROJECT_NAME}
	xenbe
	pthread
	${CONFIG_LIBRARIES}
)
