cmake_minimum_required(VERSION 3.1...3.25)
project(
	artd-gpu-engine
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

# We add an option to enable different settings when developping the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

file(GLOB_RECURSE includes "*.h" "*.inl")

file(GLOB artd-gpu-engine-sources "*.cpp" "*.c")
# cmake is NASTY !!!  https://cmake.org/cmake/help/v3.6/command/string.html#regex-specification
list(FILTER artd-gpu-engine-sources EXCLUDE REGEX ".*[/\\\]main\\.cpp$")

add_library("artd-gpu-engine" SHARED
    ${artd-gpu-engine-sources}
    ${includes}
)

target_compile_definitions( "artd-gpu-engine" PRIVATE
        "-DARTD_SUPPORT_STD_STRING"
        "-DGLM_FORCE_DEPTH_ZERO_TO_ONE"
    #    "-DGLM_FORCE_LEFT_HANDED"
        "-DBUILDING_artd_gpu_engine"
    )

if(DEV_MODE)
	# In dev mode, we load resources from the source tree, so that when we
	# dynamically edit resources (like shaders), these are correctly
	# versioned.
	target_compile_definitions(artd-gpu-engine PRIVATE
		RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
	)
else()
	# In release mode, we just load resources relatively to wherever the
	# executable is launched from, so that the binary is portable
	target_compile_definitions(artd-gpu-engine PRIVATE
		RESOURCE_DIR="./resources"
	)
endif()

target_link_libraries(artd-gpu-engine PRIVATE
    artd-jlib-base
    artd-jlib-io
    artd-jlib-util
    artd-lib-vecmath
    artd-jlib-thread
    glfw
    webgpu
    glfw3webgpu)


set_target_properties(artd-gpu-engine PROPERTIES
	CXX_STANDARD 17
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)
target_treat_all_warnings_as_errors(artd-gpu-engine)

add_executable(ArtdGpuEngineTest
	main.cpp
)

target_link_libraries(ArtdGpuEngineTest PRIVATE artd-gpu-engine)

# target_copy_webgpu_binaries(ArtdGpuEngineTest)

