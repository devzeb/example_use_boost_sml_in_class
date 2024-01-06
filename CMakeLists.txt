cmake_minimum_required(VERSION 3.22)
project(example_use_boost_sml_in_class)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/sml)

add_executable(example_use_boost_sml_in_class main.cpp)

target_link_libraries(example_use_boost_sml_in_class PRIVATE
        sml::sml
)