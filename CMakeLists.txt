cmake_minimum_required (VERSION 2.6)
set (CMAKE_CXX_STANDARD 11)
project (Chat)

find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS}
    "common"
    "external/args"
    "external/spdlog/include"
    "external/cereal/include")

# add the executable
add_executable(server server/main.cpp server/Server.cpp)
add_executable(client client/main.cpp)

target_link_libraries(server ${Boost_LIBRARIES} pthread)
target_link_libraries(client ${Boost_LIBRARIES} pthread)