

cmake_minimum_required(VERSION 3.10)


project(Matching VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)



#configure_file(TutorialConfig.h.in TutorialConfig.h) 



add_executable(Matching matching.cxx)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(Matching ${PYTHON_LIBRARIES})

#target_include_directories(Tutorial PUBLIC "${PROJECT_BINARY_DIR}")
