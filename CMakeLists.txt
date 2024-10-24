cmake_minimum_required (VERSION 3.8)

project (ChipletNetworkSim VERSION 1.0
						   DESCRIPTION "A cycle-accurate packet-parallel network simulator."
                           LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUG")
endif()
message("CMAKE_CXX_FLAGS_DEBUG value is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE value is ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

# Find Boost
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

