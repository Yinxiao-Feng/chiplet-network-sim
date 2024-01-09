cmake_minimum_required (VERSION 3.8)

project (ChipletNetworkSim VERSION 0.8
						   DESCRIPTION "A cycle-accurate packet-parallel network simulator."
                           LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUG")
endif()
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall" )
message("CMAKE_CXX_FLAGS_DEBUG value is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE value is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

find_package(Boost)
if(Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})
endif()

aux_source_directory(src SRC_FILES)
aux_source_directory(src/netrace NETRACE_FILES)
aux_source_directory(src/topologies TOPOLOGY_FILES)
add_executable (${PROJECT_NAME} ${SRC_FILES} ${NETRACE_FILES} ${TOPOLOGY_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC src src/netrace src/topologies)

if(MSVC)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:RELWITHDEBINFO>:/PROFILE>)
endif()

