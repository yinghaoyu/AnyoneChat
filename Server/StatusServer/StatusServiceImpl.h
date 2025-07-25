#pragma once

#include "message.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class ChatServer
{
  public:
    ChatServer() : host_(""), port_(""), name_(""), con_count(0) {}
    ChatServer(const ChatServer& cs)
        : host_(cs.host_),
          port_(cs.port_),
          name_(cs.name_),
          con_count(cs.con_count)
    {}
    ChatServer& operator=(const ChatServer& cs)
    {
        if (&cs == this)
        {
            return *this;
        }

        host_     = cs.host_;
        name_     = cs.name_;
        port_     = cs.port_;
        con_count = cs.con_count;
        return *this;
    }
    std::string host_;
    std::string port_;
    std::string name_;
    int         con_count;
};
class StatusServiceImpl final : public StatusService::Service
{
  public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context,
        const GetChatServerReq* request, GetChatServerRsp* reply) override;

  private:
    void       insertToken(int uid, std::string token);
    ChatServer getChatServer();
    std::unordered_map<std::string, ChatServer> servers_;
    std::mutex                                  mutex_;
};
