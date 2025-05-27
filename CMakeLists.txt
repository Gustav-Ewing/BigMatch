

cmake_minimum_required(VERSION 3.10)


project(Matching VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#for lldb debugging
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

#these 2 lines activates address sanitizer which might help with mem errors
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external)


add_executable(Greedy greedy.cxx)
add_executable(Double doubleGreedy.cxx)
add_executable(Generate generateGraph.cxx)

target_link_libraries(Double PRIVATE cereal)

# find_package(PythonLibs REQUIRED)
# include_directories(${PYTHON_INCLUDE_DIRS})
# target_link_libraries(Matching ${PYTHON_LIBRARIES})


#target_include_directories(Tutorial PUBLIC "${PROJECT_BINARY_DIR}")
