#include "StatusGrpcClient.h"
#include "ConfigMgr.h"
#include "const.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    ClientContext    context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);
    auto   stub   = pool_->getConnection();
    Status status = stub->GetChatServer(&context, request, &reply);
    Defer  defer([&stub, this]() { pool_->returnConnection(std::move(stub)); });
    if (status.ok())
    {
        return reply;
    }
    else
    {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}

StatusGrpcClient::StatusGrpcClient()
{
    auto&       gCfgMgr = ConfigMgr::Inst();
    std::string host    = gCfgMgr["StatusServer"]["Host"];
    std::string port    = gCfgMgr["StatusServer"]["Port"];
    pool_.reset(new StatusConPool(5, host, port));
}
