################################################################################
# Check packages
################################################################################

include(FindPkgConfig)

pkg_check_modules (DRM REQUIRED libdrm)
if (!DRM_FOUND)
	message(FATAL_ERROR "No libdrm found")
endif()

if(WITH_WAYLAND)
	pkg_check_modules (WAYLAND REQUIRED wayland-client)
	if(!WAYLAND_FOUND)
		message(FATAL_ERROR "No wayland-client found")
	endif()
endif()

if(WITH_IVI_EXTENSION AND WITH_WAYLAND)
	pkg_check_modules (IVI_APP ivi-application)
	if(!IVI_APP_FOUND)
		message(WARNING "No ivi-application found")
	endif()

	pkg_check_modules (ILM_CTRL ilmControl)
	if(!ILM_CTRL_FOUND)
		message(WARNING "No ilmControl found")
	endif()

	pkg_check_modules (ILM_COMMON ilmCommon)
	if(!ILM_COMMON_FOUND)
		message(WARNING "No ilmCommon found")
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
	pthread
)
