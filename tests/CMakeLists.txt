find_package(Catch2 REQUIRED)

# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(test_st_utils
	test_st_utils.cpp
	${SRCS}/st_utils.hpp
	${SRCS}/st_utils.cpp)

target_include_directories(test_st_utils PRIVATE
	${SRCS})

target_link_libraries(test_st_utils PRIVATE
	Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_st_utils)
