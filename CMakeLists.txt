

cmake_minimum_required(VERSION 3.10)


project(Matching VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#configure_file(TutorialConfig.h.in TutorialConfig.h) 

#these 2 lines activates address sanitizer which might help with mem errors
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_executable(Greedy greedy.cxx)




#target_include_directories(Tutorial PUBLIC "${PROJECT_BINARY_DIR}")
