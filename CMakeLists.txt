cmake_minimum_required (VERSION 3.8)

project ("ChipletNetworkSim")

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("CMAKE_CXX_FLAGS_DEBUG default value is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE default value is ${CMAKE_CXX_FLAGS_RELEASE}")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -pg")

find_package(Boost)
if(Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})
endif()

aux_source_directory(./src SRC_FILES)
aux_source_directory(./src/netrace NETRACE_FILES)
add_executable (${PROJECT_NAME} ${SRC_FILES} ${NETRACE_FILES})

# SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/PROFILE")

