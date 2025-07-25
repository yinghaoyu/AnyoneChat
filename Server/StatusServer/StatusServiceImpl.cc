#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "Logger.h"
#include "RedisMgr.h"
#include "const.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <climits>

std::string generate_unique_string()
{
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context,
    const GetChatServerReq* request, GetChatServerRsp* reply)
{
    LOG_INFO("gRpc GetChatServer uid: {}", request->uid());
    const auto& server = getChatServer();
    reply->set_host(server.host_);
    reply->set_port(server.port_);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string());
    insertToken(request->uid(), reply->token());
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg         = ConfigMgr::Inst();
    auto  server_list = cfg["chatservers"]["Name"];

    std::vector<std::string> words;

    std::stringstream ss(server_list);
    std::string       word;

    while (std::getline(ss, word, ','))
    {
        words.push_back(word);
    }

    for (auto& word : words)
    {
        if (cfg[word]["Name"].empty())
        {
            continue;
        }

        ChatServer server;
        server.port_           = cfg[word]["Port"];
        server.host_           = cfg[word]["Host"];
        server.name_           = cfg[word]["Name"];
        servers_[server.name_] = server;
    }
}

ChatServer StatusServiceImpl::getChatServer()
{
    LOG_INFO("Redis get chat server begin");
    std::lock_guard<std::mutex> guard(mutex_);
    auto                        minServer = servers_.begin()->second;

    auto count_str =
        RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name_);
    if (count_str.empty())
    {
        // 不存在则默认设置为最大
        minServer.con_count = INT_MAX;
    }
    else
    {
        minServer.con_count = std::stoi(count_str);
    }

    // 使用范围基于for循环
    for (auto& server : servers_)
    {

        if (server.second.name_ == minServer.name_)
        {
            continue;
        }

        auto count_str =
            RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name_);
        if (count_str.empty())
        {
            server.second.con_count = INT_MAX;
        }
        else
        {
            server.second.con_count = std::stoi(count_str);
        }

        if (server.second.con_count < minServer.con_count)
        {
            minServer = server.second;
        }
    }
    LOG_INFO("Redis get chat server finish");
    return minServer;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
    std::string uid_str   = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}
