cmake_minimum_required (VERSION 3.8)

project ("ChipletNetworkSim")

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -pg")
message("Default CMAKE_CXX_FLAGS_DEBUG value is ${CMAKE_CXX_FLAGS_DEBUG}")
message("Default CMAKE_CXX_FLAGS_RELEASE value is ${CMAKE_CXX_FLAGS_RELEASE}")
message("Default CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_C_FLAGS_RELWITHDEBINFO}")

find_package(Boost)
if(Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(./src ./src/topologies)
aux_source_directory(./src SRC_FILES)
aux_source_directory(./src/netrace NETRACE_FILES)
aux_source_directory(./src/topologies TOPOLOGY_FILES)
add_executable (${PROJECT_NAME} ${SRC_FILES} ${NETRACE_FILES} ${TOPOLOGY_FILES})

if(MSVC)
target_link_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:RELWITHDEBINFO>:/DEBUG> 
										   $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
endif()

