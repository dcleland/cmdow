cmake_minimum_required(VERSION 3.20)

project(cmdow 
    VERSION 2.0.0
    DESCRIPTION "cmdow - Command line tool to manipulate windows"
    LANGUAGES CXX)

include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

find_package(catch2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Uncomment line below for an Official build
set(OFFICIAL_BUILD 1)

string(TIMESTAMP BUILDDATE "%Y%m%d")
message("BUILDDATE ${BUILDDATE}")

# Use static runtime
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
# set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# Add binary directory so the versions.hpp is picked up.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Enable PDBs for release builds
if (MSVC)
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")
    add_compile_options("$<$<NOT:$<CONFIG:Release>>: /wd4267>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")
endif()

set(SRCS ${CMAKE_CURRENT_SOURCE_DIR})

#
# Programs start here
#
set(cmdow_HDRS
header.h
registry_wrappers.hpp
get_last_error_text.hpp
st_utils.hpp
)

set(cmdow_SRCS
args.cpp
help.cpp
main.cpp
tlist.cpp
wlist.cpp
get_last_error_text.cpp
get_counter_ids.cpp
st_utils.cpp
VERSION
version.hpp.in
)

add_executable(cmdow
    ${cmdow_HDRS}
    ${cmdow_SRCS}
    )

target_compile_definitions(cmdow PRIVATE 
    WIN32
    WINDOWS
    #UNICODE
    #_UNICODE
    )

target_link_libraries(cmdow PRIVATE
    project_options
    project_warnings
    fmt::fmt
#    gdiplus
    )

# Uncomment below to enable address sanitizer
#target_compile_options(cmdow PRIVATE 
#/fsanitize=address /Zi
#)

add_custom_target(VersionHeader
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/VersionHeader.cmake
)
add_dependencies(cmdow VersionHeader)

add_subdirectory(tests)

#
#
#
install(TARGETS cmdow
        CONFIGURATIONS Release
        RUNTIME DESTINATION release/bin)

install(TARGETS cmdow
    CONFIGURATIONS Debug
        RUNTIME DESTINATION debug/bin)

install(FILES $<TARGET_PDB_FILE:cmdow>
        CONFIGURATIONS Release
        DESTINATION release/bin OPTIONAL)

install(FILES $<TARGET_PDB_FILE:cmdow>
        CONFIGURATIONS Debug
        DESTINATION debug/bin OPTIONAL)

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
   set(TARGETBITS 32)
else()
   set(TARGETBITS 64)
endif()

message("")
if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
   set(TARGETBITS 32)
else()
   set(TARGETBITS 64)
endif()

message("cmdow configured to run on:  ${CMAKE_SYSTEM_NAME} ${TARGETBITS} bit, ${CMAKE_BUILD_TYPE} Mode")
message("cmdow will be built in:      ${CMAKE_BINARY_DIR}")
message("cmdow will be installed in:  ${CMAKE_INSTALL_PREFIX}")
message("\n")
