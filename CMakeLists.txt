cmake_minimum_required(VERSION 3.28)

project(AnyoneChat)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
 -g
 -ggdb
  )

if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()
if(spdlog_FOUND)
    add_definitions(-DSPDLOG_ENABLE_SOURCE_LOCATION)
    include_directories(${spdlog_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "spdlog not found")
endif()

if(NOT TARGET Boost)
    find_package(Boost REQUIRED)
endif()
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found")
endif()

# find_package(MySQL REQUIRED)
# if(MySQL_FOUND)
#     include_directories(${MYSQL_INCLUDE_DIR})
# else()
#   message(FATAL_ERROR "mysql not found")
# endif()

# find_package(jsoncpp REQUIRED)
# if(JSONCPP_FOUND)
#   include_directories(${JSONCPP_INCLUDE_DIRS})
# else()
#   message(FATAL_ERROR "jsoncpp not found")
# endif()

# find_package(hiredis REQUIRED)
# if(HIREDIS_FOUND)
#     include_directories(${HIREDIS_INCLUDE_DIRS})
# else()
#     message(FATAL_ERROR "hiredis not found")
# endif()
if(NOT TARGET Protobuf)
    find_package(Protobuf REQUIRED)
endif()
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Protobuf not found")
endif()

if(NOT TARGET gRPC)
    find_package(gRPC REQUIRED)
endif()
find_package(gRPC REQUIRED)
if(gRPC_FOUND)
    include_directories(${gRPC_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "gRPC not found")
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(Server/Common)

add_subdirectory(Server/Common)
add_subdirectory(Server/ChatServer)
add_subdirectory(Server/GateServer)
add_subdirectory(Server/ResourceServer)
add_subdirectory(Server/StatusServer)