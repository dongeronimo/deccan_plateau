cmake_minimum_required(VERSION 3.10)
project(DeccanPlateau VERSION 1.0)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Include FetchContent module
include(FetchContent)
# Fetch GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4 # Specify the desired version
)
FetchContent_MakeAvailable(glfw)
# Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.0 # Specify the desired version
)
FetchContent_MakeAvailable(glm)
# Add the executable
add_executable(deccan-plateau-demo 
    deccan-plateau-demo.cpp 
    deccan-plateau-demo.h 
    my-vk.cpp 
    my-vk.h
    my-vk-extensions.cpp
    my-vk-extensions.h
    my-vk-validationLayers.cpp
    my-vk-validationLayers.h
	my-vk-shaders.cpp
	my-vk-shaders.h)
# Include GLM headers
target_include_directories(deccan-plateau-demo PRIVATE ${glm_SOURCE_DIR})
# Link Vulkan library
target_link_libraries(deccan-plateau-demo Vulkan::Vulkan glfw)
target_compile_definitions(deccan-plateau-demo PRIVATE 
    #PRINT_ALLOCATIONS #If present enables printing of memory operation at the allocation callback
    VK_DEBUG_LEVEL=VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT # See VkDebugUtilsMessageSeverityFlagBitsEXT @vulkan_core.h
    MAX_FRAMES_IN_FLIGHT=2
    GLM_FORCE_RADIANS #GLM must use radians everywhere
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES #Force glm vector and matrix types to be aligned
)
target_compile_features(deccan-plateau-demo PRIVATE cxx_std_17)

# Post-Build scripts for the shaders,
set(SCRIPT_DIR "${CMAKE_SOURCE_DIR}")
if(WIN32)
    # Add a post-build command to run the .bat file
    add_custom_command(
        TARGET deccan-plateau-demo
        POST_BUILD
         COMMAND "${SCRIPT_DIR}/compile_shaders.bat" "${SCRIPT_DIR}" 
    )
else()
    add_custom_command(
        TARGET deccan-plateau-demo
        POST_BUILD
        COMMAND "${SCRIPT_DIR}/compile_shaders.sh" "${SCRIPT_DIR}" 
    )
endif()