#include "ChatServiceImpl.h"
#include "Session.h"
#include "Logger.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "MysqlMgr.h"
#include "utils.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>

ChatServiceImpl::ChatServiceImpl() {}

Status ChatServiceImpl::NotifyAddFriend(
    ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
    // 查找用户是否在本服务器
    auto touid   = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    });

    // 用户不在内存中则直接返回
    if (session == nullptr)
    {
        LOG_ERROR("NotifyAddFriend touid not in memory, fromuid: {}, touid: {}",
                  request->applyuid(), touid);
        return Status::OK;
    }
    // 在内存中则直接发送通知对方
    Json::Value rtvalue;
    rtvalue["error"]    = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"]     = request->name();
    rtvalue["desc"]     = request->desc();
    rtvalue["icon"]     = request->icon();
    rtvalue["sex"]      = request->sex();
    rtvalue["nick"]     = request->nick();

    std::string return_str = rtvalue.toStyledString();
    LOG_INFO("NotifyAddFriend session send begin, fromuid: {}, touid: {}",
             request->applyuid(), touid);
    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
    LOG_INFO("NotifyAddFriend session send finish, fromuid: {}, touid: {}",
             request->applyuid(), touid);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(
    ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* reply)
{
    // 查找用户是否在本服务器
    auto touid   = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
    });

    // 用户不在内存中则直接返回
    if (session == nullptr)
    {
        LOG_ERROR(
            "NotifyAuthFriend touid not in memory, fromuid: {}, touid: {}",
            request->fromuid(), touid);
        return Status::OK;
    }
    // 在内存中则直接发送通知对方
    Json::Value rtvalue;
    rtvalue["error"]   = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"]   = request->touid();

    std::string base_key  = USER_BASE_INFO + std::to_string(fromuid);
    auto        user_info = std::make_shared<UserInfo>();
    bool        b_info    = GetBaseInfo(base_key, fromuid, user_info);
    if (b_info)
    {
        rtvalue["name"] = user_info->name_;
        rtvalue["nick"] = user_info->nick_;
        rtvalue["icon"] = user_info->icon_;
        rtvalue["sex"]  = user_info->sex_;
    }
    else
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    auto chat_time = getCurrentTimestamp();
    for (auto& msg : request->textmsgs())
    {
        Json::Value chat;
        chat["sender"]      = msg.sender_id();
        chat["msg_id"]      = msg.msg_id();
        chat["thread_id"]   = msg.thread_id();
        chat["unique_id"]   = msg.unique_id();
        chat["msg_content"] = msg.msgcontent();
        chat["chat_time"]   = chat_time;
        chat["status"]      = msg.status();
        rtvalue["chat_datas"].append(chat);
    }

    std::string return_str = rtvalue.toStyledString();
    LOG_INFO("NotifyAuthFriend session send begin, fromuid: {}, touid: {}",
             request->fromuid(), touid);
    session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
    LOG_INFO("NotifyAuthFriend session send finish, fromuid: {}, touid: {}",
             request->fromuid(), touid);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
    const TextChatMsgReq* request, TextChatMsgRsp* reply)
{
    // 查找用户是否在本服务器
    auto touid   = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);
    reply->set_error(ErrorCodes::Success);

    // 用户不在内存中则直接返回
    if (session == nullptr)
    {
        LOG_ERROR(
            "NotifyTextChatMsg touid not in memory, fromuid: {}, touid: {}",
            request->fromuid(), touid);
        return Status::OK;
    }

    // 在内存中则直接发送通知对方
    Json::Value rtvalue;
    rtvalue["error"]     = ErrorCodes::Success;
    rtvalue["fromuid"]   = request->fromuid();
    rtvalue["touid"]     = request->touid();
    rtvalue["thread_id"] = request->thread_id();

    // 将聊天数据组织为数组
    Json::Value text_array;
    for (auto& msg : request->textmsgs())
    {
        Json::Value element;
        element["content"]    = msg.msgcontent();
        element["unique_id"]  = msg.unique_id();
        element["message_id"] = msg.msg_id();
        element["chat_time"]  = msg.chat_time();
        text_array.append(element);
    }
    rtvalue["chat_datas"] = text_array;

    std::string return_str = rtvalue.toStyledString();
    LOG_INFO("NotifyTextChatMsg session send begin, fromuid: {}, touid: {}",
             request->fromuid(), touid);
    session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    LOG_INFO("NotifyTextChatMsg session send finsh, fromuid: {}, touid: {}",
             request->fromuid(), touid);
    return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(
    std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    // 优先查redis中查询用户信息
    std::string info_str = "";
    bool        b_base   = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base)
    {
        Json::Reader reader;
        Json::Value  root;
        reader.parse(info_str, root);
        userinfo->uid_   = root["uid"].asInt();
        userinfo->name_  = root["name"].asString();
        userinfo->pwd_   = root["pwd"].asString();
        userinfo->email_ = root["email"].asString();
        userinfo->nick_  = root["nick"].asString();
        userinfo->desc_  = root["desc"].asString();
        userinfo->sex_   = root["sex"].asInt();
        userinfo->icon_  = root["icon"].asString();
        LOG_INFO("Redis get user info, uid: {}, name, {}, pwd: {}, email: {}, "
                 "nick: {}, desc: {}, sex: {}, icon: {}",
                 uid, userinfo->name_, userinfo->pwd_, userinfo->email_,
                 userinfo->nick_, userinfo->desc_, userinfo->sex_, userinfo->icon_);
    }
    else
    {
        // redis中没有则查询mysql
        // 查询数据库
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr)
        {
            LOG_ERROR("Mysql get user info failed, uid: {}", uid);
            return false;
        }

        LOG_INFO("Mysql get user info, uid: {}, name: {}, pwd: {}, "
                 "email: {}, nick: {}, desc: {}, sex: {}, icon: {}",
                 uid, user_info->name_, user_info->pwd_, user_info->email_,
                 user_info->nick_, user_info->desc_, user_info->sex_,
                 user_info->icon_);

        userinfo = user_info;

        // 将数据库内容写入redis缓存
        Json::Value redis_root;
        redis_root["uid"]   = uid;
        redis_root["pwd"]   = userinfo->pwd_;
        redis_root["name"]  = userinfo->name_;
        redis_root["email"] = userinfo->email_;
        redis_root["nick"]  = userinfo->nick_;
        redis_root["desc"]  = userinfo->desc_;
        redis_root["sex"]   = userinfo->sex_;
        redis_root["icon"]  = userinfo->icon_;
        RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
    }

    return true;
}

Status ChatServiceImpl::NotifyKickUser(::grpc::ServerContext* context,
    const KickUserReq* request, KickUserRsp* reply)
{
    // 查找用户是否在本服务器
    auto uid     = request->uid();
    auto session = UserMgr::GetInstance()->GetSession(uid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_uid(request->uid());
    });

    // 用户不在内存中则直接返回
    if (session == nullptr)
    {
        LOG_ERROR("NotifyKickUser user not in memory, uid: {}", uid);
        return Status::OK;
    }
    LOG_INFO("NotifyTextChatMsg session send begin, uid: {}", uid);
    // 在内存中则直接发送通知对方
    session->NotifyOffline(uid);
    LOG_INFO("NotifyTextChatMsg session send finish, uid: {}", uid);
    // 清除旧的连接
    server_->CleanSession(session->GetSessionId());

    return Status::OK;
}

void ChatServiceImpl::RegisterServer(std::shared_ptr<ChatServer> pServer)
{
    server_ = pServer;
}
