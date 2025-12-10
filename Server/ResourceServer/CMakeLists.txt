file(GLOB ChatServer_SRCS
    "*.cc"
)

add_executable(ResourceServer ${ChatServer_SRCS})
target_link_libraries(ResourceServer
                      Common
                      pthread
                      boost_filesystem
                      boost_system
                      protobuf
                      gRPC::grpc++_reflection
                      gRPC::grpc++
                      jsoncpp
                      mysqlclient
                      hiredis
                      spdlog::spdlog
                      )
