################################################################################
# Includes
################################################################################

################################################################################
# Sources
################################################################################

if(WITH_DRM)
	add_subdirectory(drm)
endif()

if(WITH_WAYLAND)
	add_subdirectory(wayland)
endif()

set(SOURCES
	BuffersStorage.cpp
	DisplayBackend.cpp
	DisplayCommandHandler.cpp
)

################################################################################
# Targets
################################################################################

add_library(display STATIC ${SOURCES})

################################################################################
# Libraries
################################################################################

if(WITH_DRM)
	target_link_libraries(display display_drm)
endif()

if(WITH_WAYLAND)
	target_link_libraries(display display_wayland)
endif()

target_include_directories(display PUBLIC . )
