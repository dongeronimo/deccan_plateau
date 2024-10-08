cmake_minimum_required(VERSION 3.10)
project(DeccanPlateau VERSION 1.0)
# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Set policy CMP0077
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW) #sorcery to allow my fetched libpng to link with my fetched zlib
endif()
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

# Fetch Assimp
# Disable all exporters
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# Disable all importers by default
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# Enable FBX importer
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
# Enable glTF importer
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
# Optionally, disable features you don't need
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.2 # or the version you need
)
FetchContent_MakeAvailable(assimp)

# Link zlib to Assimp
set(ASSIMP_USE_EXTERNAL_ZLIB ON)
set(ASSIMP_BUILD_ZLIB OFF)
set(ASSIMP_BUILD_TESTS OFF)
target_link_libraries(assimp  zlibstatic)

file(GLOB utils_files "utils/*.cpp" "utils/*.h")
file(GLOB app_files "app/*.cpp" "app/*.h")
file(GLOB vk_files "vk/*.cpp" "vk/*.h")
file(GLOB entities_files "entities/*.cpp" "entities/*.h")
file(GLOB io_files "io/*.cpp" "io/*.h")
file(GLOB gpu_picker_files "gpu-picking/*.cpp" "gpu-picking/*.h")

source_group("utils" FILES ${utils_files})
source_group("app" FILES ${app_files})
source_group("vk" FILES ${vk_files})
source_group("entities" FILES ${entities_files})
source_group("io" FILES ${io_files})
source_group("gpu_picker" FILES ${gpu_picker_files})
# Add the executable
add_executable(deccan-plateau-demo 
    ${utils_files}
    ${app_files}
    ${vk_files}
    ${entities_files}
    ${io_files}
    ${gpu_picker_files}
)
target_include_directories(deccan-plateau-demo PRIVATE .)
# Include GLM headers
target_include_directories(deccan-plateau-demo PRIVATE ${glm_SOURCE_DIR})
# Link Vulkan library
target_link_libraries(deccan-plateau-demo PRIVATE 
    Vulkan::Vulkan 
    glfw
    assimp)
target_compile_definitions(deccan-plateau-demo PRIVATE 
    #PRINT_ALLOCATIONS #If present enables printing of memory operation at the allocation callback
    VK_DEBUG_LEVEL=VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT # See VkDebugUtilsMessageSeverityFlagBitsEXT @vulkan_core.h
    MAX_FRAMES_IN_FLIGHT=2
    GLM_FORCE_RADIANS #GLM must use radians everywhere
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES #Force glm vector and matrix types to be aligned
    GLM_FORCE_DEPTH_ZERO_TO_ONE
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
    add_custom_command(
        TARGET deccan-plateau-demo
        POST_BUILD
         COMMAND "${SCRIPT_DIR}/copy_assets.bat" "${SCRIPT_DIR}" 
    )
endif()