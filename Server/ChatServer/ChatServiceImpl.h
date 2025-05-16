#pragma once

#include "message.grpc.pb.h"
#include "message.pb.h"
#include "data.h"
#include "CServer.h"

#include <grpcpp/grpcpp.h>
#include <memory>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::KickUserReq;
using message::KickUserRsp;
using message::TextChatData;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;

class ChatServiceImpl final : public ChatService::Service
{
  public:
    ChatServiceImpl();
    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;

    Status NotifyAuthFriend(ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;

    Status NotifyTextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;

    bool GetBaseInfo(
        std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

    // 接受rpc踢人请求
    Status NotifyKickUser(::grpc::ServerContext* context,
        const KickUserReq* request, KickUserRsp* response) override;

    void RegisterServer(std::shared_ptr<CServer> pServer);

  private:
    std::shared_ptr<CServer> _p_server;
};
