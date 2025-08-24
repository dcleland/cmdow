# This CMake script reads the version information from a VERSION file
# Build variables locally so configure_file seems them
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/VERSION" VERSION_INFO)
string(TIMESTAMP BUILDDATE "%Y%m%d")# configure_file(${CMAKE_CURRENT_LIST_DIR}/version.hpp.in ${CMAKE_CURRENT_LIST_DIR}/version.hpp )
configure_file(${CMAKE_CURRENT_LIST_DIR}/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.hpp )
