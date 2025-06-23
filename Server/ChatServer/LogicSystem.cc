#include "LogicSystem.h"
#include "ChatGrpcClient.h"
#include "ChatServer.h"
#include "ConfigMgr.h"
#include "Logger.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "const.h"

#include <string>

using namespace std;

LogicSystem::LogicSystem() : stopped_(false), server_(nullptr)
{
    RegisterCallBacks();
    for (int i = 0; i < 4; ++i)
    {
        threads_.emplace_back(&LogicSystem::DealMsg, this);
    }
}

void LogicSystem::Shutdown()
{
    stopped_ = true;
    consume_.notify_all();
    for (auto& t : threads_)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void LogicSystem::PostMsgToQue(shared_ptr<LogicNode> msg)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopped_)
    {
        return;
    }
    msg_que_.emplace_back(msg);
    consume_.notify_one();
}

void LogicSystem::SetServer(std::shared_ptr<ChatServer> pserver)
{
    server_ = pserver;
}

void LogicSystem::DealMsg()
{
    for (;;)
    {
        std::vector<std::shared_ptr<LogicNode>> msgs;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            consume_.wait(
                lock, [this] { return !msg_que_.empty() || stopped_; });

            if (stopped_ && msg_que_.empty())
            {
                break;
            }

            std::swap(msgs, msg_que_);
        }

        for (auto& msg_node : msgs)
        {
            LOG_INFO("handle msg, id:{}", msg_node->recv_node_->msg_id_);
            auto call_back_iter =
                fun_callbacks_.find(msg_node->recv_node_->msg_id_);
            if (call_back_iter == fun_callbacks_.end())
            {
                LOG_ERROR("handle msg, handler not found, msg id:{}",
                          msg_node->recv_node_->msg_id_);
                continue;
            }
            call_back_iter->second(msg_node->session_,
                msg_node->recv_node_->msg_id_,
                std::string(msg_node->recv_node_->data_,
                    msg_node->recv_node_->cur_len_));
        }
    }
}

void LogicSystem::RegisterCallBacks()
{
    fun_callbacks_[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler,
        this,
        placeholders::_1,
        placeholders::_2,
        placeholders::_3);

    fun_callbacks_[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo,
        this,
        placeholders::_1,
        placeholders::_2,
        placeholders::_3);

    fun_callbacks_[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply,
        this,
        placeholders::_1,
        placeholders::_2,
        placeholders::_3);

    fun_callbacks_[ID_AUTH_FRIEND_REQ] =
        std::bind(&LogicSystem::AuthFriendApply,
            this,
            placeholders::_1,
            placeholders::_2,
            placeholders::_3);

    fun_callbacks_[ID_TEXT_CHAT_MSG_REQ] =
        std::bind(&LogicSystem::DealChatTextMsg,
            this,
            placeholders::_1,
            placeholders::_2,
            placeholders::_3);

    fun_callbacks_[ID_HEART_BEAT_REQ] =
        std::bind(&LogicSystem::HeartBeatHandler,
            this,
            placeholders::_1,
            placeholders::_2,
            placeholders::_3);

    fun_callbacks_[ID_LOAD_CHAT_THREAD_REQ] =
        std::bind(&LogicSystem::GetUserThreadsHandler,
            this,
            placeholders::_1,
            placeholders::_2,
            placeholders::_3);

    fun_callbacks_[ID_CREATE_PRIVATE_CHAT_REQ] =
        std::bind(&LogicSystem::CreatePrivateChat,
            this,
            placeholders::_1,
            placeholders::_2,
            placeholders::_3);
}

void LogicSystem::LoginHandler(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid   = root["uid"].asInt();
    auto token = root["token"].asString();
    LOG_INFO("LoginHandler use login in, uid: {}, token: {}", uid, token);

    Json::Value rtvalue;
    Defer       defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
    });

    // 从redis获取用户token是否正确
    std::string uid_str     = std::to_string(uid);
    std::string token_key   = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool        success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success)
    {
        LOG_INFO("LoginHandler user token not exist, uid: {}", uid);
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    if (token_value != token)
    {
        LOG_INFO("LoginHandler user token not match, uid: {}, token: {}, real "
                 "token: {}",
                 uid, token, token_value);
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }

    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key  = USER_BASE_INFO + uid_str;
    auto        user_info = std::make_shared<UserInfo>();
    bool        b_base    = GetBaseInfo(base_key, uid, user_info);
    if (!b_base)
    {
        LOG_INFO("LoginHandler user not exists, uid: {}, token: {}", uid,
                 token);
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    rtvalue["uid"]   = uid;
    rtvalue["pwd"]   = user_info->pwd_;
    rtvalue["name"]  = user_info->name_;
    rtvalue["email"] = user_info->email_;
    rtvalue["nick"]  = user_info->nick_;
    rtvalue["desc"]  = user_info->desc_;
    rtvalue["sex"]   = user_info->sex_;
    rtvalue["icon"]  = user_info->icon_;

    // 从数据库获取申请列表
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;

    auto b_apply = GetFriendApplyInfo(uid, apply_list);

    if (b_apply)
    {
        for (auto& apply : apply_list)
        {
            Json::Value obj;
            obj["name"]   = apply->name_;
            obj["uid"]    = apply->uid_;
            obj["icon"]   = apply->icon_;
            obj["nick"]   = apply->nick_;
            obj["sex"]    = apply->sex_;
            obj["desc"]   = apply->desc_;
            obj["status"] = apply->status_;
            rtvalue["apply_list"].append(obj);
        }
    }

    // 获取好友列表
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = GetFriendList(uid, friend_list);
    for (auto& friend_ele : friend_list)
    {
        Json::Value obj;
        obj["name"] = friend_ele->name_;
        obj["uid"]  = friend_ele->uid_;
        obj["icon"] = friend_ele->icon_;
        obj["nick"] = friend_ele->nick_;
        obj["sex"]  = friend_ele->sex_;
        obj["desc"] = friend_ele->desc_;
        obj["back"] = friend_ele->back_;
        rtvalue["friend_list"].append(obj);
    }

    auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
    {
        // 此处添加分布式锁，让该线程独占登录
        // 拼接用户ip对应的key
        auto lock_key   = LOCK_PREFIX + uid_str;
        auto identifier = RedisMgr::GetInstance()->acquireLock(
            lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
        // 利用defer解锁
        Defer defer2([this, identifier, lock_key]() {
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });
        // 此处判断该用户是否在别处或者本服务器登录

        std::string uid_ip_value = "";
        auto        uid_ip_key   = USERIPPREFIX + uid_str;
        bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);
        // 说明用户已经登录了，此处应该踢掉之前的用户登录状态
        if (b_ip)
        {
            LOG_INFO("LoginHandler user already login, uid: {}, ip: {}", uid,
                     uid_ip_value);
            // 获取当前服务器ip信息
            auto& cfg       = ConfigMgr::Inst();
            auto  self_name = cfg["SelfServer"]["Name"];
            // 如果之前登录的服务器和当前相同，则直接在本服务器踢掉
            if (uid_ip_value == self_name)
            {
                LOG_INFO("LoginHandler user already login in same server, uid: "
                         "{}, lastip: {}",
                         uid_ip_value);
                // 查找旧有的连接
                auto old_session = UserMgr::GetInstance()->GetSession(uid);

                // 此处应该发送踢人消息
                if (old_session)
                {
                    LOG_INFO("LoginHandler user already login in same server, "
                             "uid: {}, old_session: {}",
                             uid, old_session->GetSessionId());
                    old_session->NotifyOffline(uid);
                    // 清除旧的连接
                    server_->CleanSession(old_session->GetSessionId());
                }
            }
            else
            {
                // 如果不是本服务器，则通知grpc通知其他服务器踢掉
                LOG_INFO("LoginHandler user already login in other server, "
                         "uid: {}, lastip: {}",
                         uid, uid_ip_value);
                KickUserReq kick_req;
                kick_req.set_uid(uid);
                ChatGrpcClient::GetInstance()->NotifyKickUser(
                    uid_ip_value, kick_req);
            }
        }

        // session绑定用户uid
        session->SetUserId(uid);
        // 为用户设置登录ip server的名字
        std::string ipkey = USERIPPREFIX + uid_str;
        RedisMgr::GetInstance()->Set(ipkey, server_name);
        // uid和session绑定管理,方便以后踢人操作
        UserMgr::GetInstance()->SetUserSession(uid, session);
        std::string uid_session_key = USER_SESSION_PREFIX + uid_str;
        RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());
    }

    return;
}

void LogicSystem::SearchInfo(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid_str = root["uid"].asString();
    LOG_INFO("user SearchInfo uid: {}", uid_str);

    Json::Value rtvalue;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_SEARCH_USER_RSP);
    });

    bool b_digit = isPureDigit(uid_str);
    if (b_digit)
    {
        GetUserByUid(uid_str, rtvalue);
    }
    else
    {
        GetUserByName(uid_str, rtvalue);
    }
    return;
}

void LogicSystem::AddFriendApply(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid       = root["uid"].asInt();
    auto applyname = root["applyname"].asString();
    auto bakname   = root["bakname"].asString();
    auto touid     = root["touid"].asInt();

    LOG_INFO(
        "user add friend apply, uid: {}, applyname: {}, touid: {}, bakname: {}",
        uid, applyname, touid, bakname);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_ADD_FRIEND_RSP);
    });

    // 先更新数据库
    MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

    // 查询redis 查找touid对应的server ip
    auto        to_str      = std::to_string(touid);
    auto        to_ip_key   = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool        b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        LOG_INFO("user add friend apply, touid not login, uid: {}, touid: {}",
                 uid, touid);
        return;
    }

    auto& cfg       = ConfigMgr::Inst();
    auto  self_name = cfg["SelfServer"]["Name"];

    std::string base_key   = USER_BASE_INFO + std::to_string(uid);
    auto        apply_info = std::make_shared<UserInfo>();
    bool        b_info     = GetBaseInfo(base_key, uid, apply_info);

    // 直接通知对方有申请消息
    if (to_ip_value == self_name)
    {
        auto session = UserMgr::GetInstance()->GetSession(touid);
        if (session)
        {
            LOG_INFO("user add friend apply, touid at same chat server, uid: "
                     "{}, touid: {}",
                     uid, touid);
            // 在内存中则直接发送通知对方
            Json::Value notify;
            notify["error"]    = ErrorCodes::Success;
            notify["applyuid"] = uid;
            notify["name"]     = applyname;
            notify["desc"]     = "";
            if (b_info)
            {
                notify["icon"] = apply_info->icon_;
                notify["sex"]  = apply_info->sex_;
                notify["nick"] = apply_info->nick_;
            }
            std::string return_str = notify.toStyledString();
            session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
        }

        return;
    }

    LOG_INFO(
        "user add friend apply, touid at other chat server, uid: {}, touid: {}",
        uid, touid);

    AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_touid(touid);
    add_req.set_name(applyname);
    add_req.set_desc("");
    if (b_info)
    {
        add_req.set_icon(apply_info->icon_);
        add_req.set_sex(apply_info->sex_);
        add_req.set_nick(apply_info->nick_);
    }

    // 发送通知
    ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(
    SessionPtr session, const short& msg_id, const string& msg_data)
{

    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);

    auto uid       = root["fromuid"].asInt();
    auto touid     = root["touid"].asInt();
    auto back_name = root["back"].asString();

    LOG_INFO("user auth friend apply, uid: {}, touid: {}, back_name: {}", uid,
             touid, back_name);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    auto user_info   = std::make_shared<UserInfo>();

    std::string base_key = USER_BASE_INFO + std::to_string(touid);
    bool        b_info   = GetBaseInfo(base_key, touid, user_info);
    if (b_info)
    {
        rtvalue["name"] = user_info->name_;
        rtvalue["nick"] = user_info->nick_;
        rtvalue["icon"] = user_info->icon_;
        rtvalue["sex"]  = user_info->sex_;
        rtvalue["uid"]  = touid;
    }
    else
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_AUTH_FRIEND_RSP);
    });

    // 先更新数据库
    MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);

    // 更新数据库添加好友
    MysqlMgr::GetInstance()->AddFriend(uid, touid, back_name);

    // 查询redis 查找touid对应的server ip
    auto        to_str      = std::to_string(touid);
    auto        to_ip_key   = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool        b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        LOG_INFO("user auth friend apply, touid not login, uid: {}, touid: {}",
                 uid, touid);
        return;
    }

    auto& cfg       = ConfigMgr::Inst();
    auto  self_name = cfg["SelfServer"]["Name"];
    // 直接通知对方有认证通过消息
    if (to_ip_value == self_name)
    {
        auto session = UserMgr::GetInstance()->GetSession(touid);
        if (session)
        {
            LOG_INFO("user auth friend apply, touid at same chat server, uid: "
                     "{}, touid: {}",
                     uid, touid);
            // 在内存中则直接发送通知对方
            Json::Value notify;
            notify["error"]       = ErrorCodes::Success;
            notify["fromuid"]     = uid;
            notify["touid"]       = touid;
            std::string base_key  = USER_BASE_INFO + std::to_string(uid);
            auto        user_info = std::make_shared<UserInfo>();
            bool        b_info    = GetBaseInfo(base_key, uid, user_info);
            if (b_info)
            {
                notify["name"] = user_info->name_;
                notify["nick"] = user_info->nick_;
                notify["icon"] = user_info->icon_;
                notify["sex"]  = user_info->sex_;
            }
            else
            {
                notify["error"] = ErrorCodes::UidInvalid;
            }

            std::string return_str = notify.toStyledString();
            session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
        }

        return;
    }

    LOG_INFO("user auth friend apply, touid at other chat server, uid: {}, "
             "touid: {}",
             uid, touid);
    AuthFriendReq auth_req;
    auth_req.set_fromuid(uid);
    auth_req.set_touid(touid);

    // 发送通知
    ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);

    auto uid   = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();

    const Json::Value arrays = root["text_array"];

    Json::Value rtvalue;
    rtvalue["error"]      = ErrorCodes::Success;
    rtvalue["text_array"] = arrays;
    rtvalue["fromuid"]    = uid;
    rtvalue["touid"]      = touid;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
    });

    // 查询redis 查找touid对应的server ip
    auto        to_str      = std::to_string(touid);
    auto        to_ip_key   = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool        b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip)
    {
        LOG_INFO("user text chat msg, touid not login, fromuid: {}, touid: {}",
                 uid, touid);
        return;
    }

    auto& cfg       = ConfigMgr::Inst();
    auto  self_name = cfg["SelfServer"]["Name"];
    // 直接通知对方有认证通过消息
    if (to_ip_value == self_name)
    {
        auto peer = UserMgr::GetInstance()->GetSession(touid);
        if (peer)
        {
            LOG_INFO("user send msg, touid at same chat server, fromuid: {}, "
                     "touid: {}, session: {}, peer: {}",
                     uid, touid, session->GetSessionId(), peer->GetSessionId());
            // 在内存中则直接发送通知对方
            std::string msg = rtvalue.toStyledString();
            peer->Send(msg, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        }

        return;
    }

    LOG_INFO(
        "user send msg, touid at other chat server, fromuid: {}, touid: {}",
        uid, touid);

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(uid);
    text_msg_req.set_touid(touid);
    for (const auto& txt_obj : arrays)
    {
        auto content = txt_obj["content"].asString();
        auto msgid   = txt_obj["msgid"].asString();

        LOG_INFO("msgid: {}, content: {}", msgid, content);

        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);
    }

    // 发送通知
    ChatGrpcClient::GetInstance()->NotifyTextChatMsg(
        to_ip_value, text_msg_req, rtvalue);
}

void LogicSystem::HeartBeatHandler(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid = root["fromuid"].asInt();
    LOG_INFO("recv heart msg, uid: {}", uid);
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::isPureDigit(const std::string& str)
{
    for (char c : str)
    {
        if (!std::isdigit(c))
        {
            return false;
        }
    }
    return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = USER_BASE_INFO + uid_str;

    // 优先查redis中查询用户信息
    std::string info_str = "";
    bool        b_base   = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base)
    {
        Json::Reader reader;
        Json::Value  root;
        reader.parse(info_str, root);
        auto uid   = root["uid"].asInt();
        auto name  = root["name"].asString();
        auto pwd   = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick  = root["nick"].asString();
        auto desc  = root["desc"].asString();
        auto sex   = root["sex"].asInt();
        auto icon  = root["icon"].asString();

        LOG_INFO("Redis get user info, uid: {}, name, {}, pwd: {}, email: {}, "
                 "nick: {}, desc: {}, sex: {}, icon: {}",
                 uid, name, pwd, email, nick, desc, sex, icon);

        rtvalue["uid"]   = uid;
        rtvalue["pwd"]   = pwd;
        rtvalue["name"]  = name;
        rtvalue["email"] = email;
        rtvalue["nick"]  = nick;
        rtvalue["desc"]  = desc;
        rtvalue["sex"]   = sex;
        rtvalue["icon"]  = icon;
        return;
    }

    LOG_INFO("Redis get user info failed, key: {}", base_key);
    LOG_INFO("Mysql get user info, uid: {}", uid_str);

    auto uid = std::stoi(uid_str);
    // redis中没有则查询mysql
    // 查询数据库
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info                           = MysqlMgr::GetInstance()->GetUser(uid);
    if (user_info == nullptr)
    {
        LOG_ERROR("Mysql get user info failed, uid: {}", uid);
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    LOG_INFO("Mysql get user info success, uid: {}, name: {}, pwd: {}, "
             "email: {}, nick: {}, desc: {}, sex: {}, icon: {}",
             uid, user_info->name_, user_info->pwd_, user_info->email_,
             user_info->nick_, user_info->desc_, user_info->sex_, user_info->icon_);

    // 将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"]   = user_info->uid_;
    redis_root["pwd"]   = user_info->pwd_;
    redis_root["name"]  = user_info->name_;
    redis_root["email"] = user_info->email_;
    redis_root["nick"]  = user_info->nick_;
    redis_root["desc"]  = user_info->desc_;
    redis_root["sex"]   = user_info->sex_;
    redis_root["icon"]  = user_info->icon_;

    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

    // 返回数据
    rtvalue["uid"]   = user_info->uid_;
    rtvalue["pwd"]   = user_info->pwd_;
    rtvalue["name"]  = user_info->name_;
    rtvalue["email"] = user_info->email_;
    rtvalue["nick"]  = user_info->nick_;
    rtvalue["desc"]  = user_info->desc_;
    rtvalue["sex"]   = user_info->sex_;
    rtvalue["icon"]  = user_info->icon_;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = NAME_INFO + name;

    // 优先查redis中查询用户信息
    std::string info_str = "";
    bool        b_base   = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base)
    {
        Json::Reader reader;
        Json::Value  root;
        reader.parse(info_str, root);
        auto uid   = root["uid"].asInt();
        auto name  = root["name"].asString();
        auto pwd   = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick  = root["nick"].asString();
        auto desc  = root["desc"].asString();
        auto sex   = root["sex"].asInt();

        LOG_INFO("Redis get user info, uid: {}, name, {}, pwd: {}, email: {}, "
                 "nick: {}, desc: {}, sex: {}",
                 uid, name, pwd, email, nick, desc, sex);

        rtvalue["uid"]   = uid;
        rtvalue["pwd"]   = pwd;
        rtvalue["name"]  = name;
        rtvalue["email"] = email;
        rtvalue["nick"]  = nick;
        rtvalue["desc"]  = desc;
        rtvalue["sex"]   = sex;
        return;
    }
    LOG_INFO("Redis get user info failed, key: {}", base_key);
    LOG_INFO("Mysql get user info name: {}", name);
    // redis中没有则查询mysql
    // 查询数据库
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::GetInstance()->GetUser(name);
    if (user_info == nullptr)
    {
        LOG_ERROR("Mysql get user info failed, uid: {}", name);
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    LOG_INFO("Mysql get user info success, uid: {}, name: {}, pwd: {}, "
             "email: {}, nick: {}, desc: {}, sex: {}, icon: {}",
             user_info->uid_, user_info->name_, user_info->pwd_, user_info->email_,
             user_info->nick_, user_info->desc_, user_info->sex_, user_info->icon_);

    // 将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"]   = user_info->uid_;
    redis_root["pwd"]   = user_info->pwd_;
    redis_root["name"]  = user_info->name_;
    redis_root["email"] = user_info->email_;
    redis_root["nick"]  = user_info->nick_;
    redis_root["desc"]  = user_info->desc_;
    redis_root["sex"]   = user_info->sex_;

    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

    // 返回数据
    rtvalue["uid"]   = user_info->uid_;
    rtvalue["pwd"]   = user_info->pwd_;
    rtvalue["name"]  = user_info->name_;
    rtvalue["email"] = user_info->email_;
    rtvalue["nick"]  = user_info->nick_;
    rtvalue["desc"]  = user_info->desc_;
    rtvalue["sex"]   = user_info->sex_;
}

bool LogicSystem::GetBaseInfo(
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
        LOG_INFO("Redis get user info succeed, uid: {}, name: {}, pwd: {}, "
                 "email: {}, icon: {}",
                 userinfo->uid_, userinfo->name_, userinfo->pwd_, userinfo->email_,
                 userinfo->icon_);
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

        LOG_INFO("Mysql get user info succeed, uid: {}, name: {}, pwd: {}, "
                 "email: {}, icon: {}",
                 user_info->uid_, user_info->name_, user_info->pwd_,
                 user_info->email_, user_info->icon_);

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

bool LogicSystem::GetFriendApplyInfo(
    int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list)
{
    // 从mysql获取好友申请列表
    return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(
    int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
    // 从mysql获取好友列表
    return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

void LogicSystem::GetUserThreadsHandler(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid     = root["uid"].asInt();
    int  last_id = root["thread_id"].asInt();
    LOG_INFO("get uid threads: {} ", uid);

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"]   = uid;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_LOAD_CHAT_THREAD_RSP);
    });

    std::vector<std::shared_ptr<ChatThreadInfo>> threads;

    int  page_size    = 10;
    bool load_more    = false;
    int  next_last_id = 0;

    bool res = GetUserThreads(
        uid, last_id, page_size, threads, load_more, next_last_id);

    if (!res)
    {
        LOG_ERROR("Mysql get user threads failed, uid: {}", uid);
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    rtvalue["load_more"]    = load_more;
    rtvalue["next_last_id"] = (int)next_last_id;

    for (auto& thread : threads)
    {
        Json::Value thread_value;
        thread_value["thread_id"] = int(thread->_thread_id);
        thread_value["type"]      = thread->_type;
        thread_value["user1_id"]  = thread->_user1_id;
        thread_value["user2_id"]  = thread->_user2_id;
        rtvalue["threads"].append(thread_value);
    }
}

bool LogicSystem::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
    std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore,
    int& nextLastId)
{
    return MysqlMgr::GetInstance()->GetUserThreads(
        userId, lastId, pageSize, threads, loadMore, nextLastId);
}

void LogicSystem::CreatePrivateChat(
    SessionPtr session, const short& msg_id, const string& msg_data)
{
    Json::Reader reader;
    Json::Value  root;
    reader.parse(msg_data, root);
    auto uid      = root["uid"].asInt();
    auto other_id = root["other_id"].asInt();

    Json::Value rtvalue;
    rtvalue["error"]    = ErrorCodes::Success;
    rtvalue["uid"]      = uid;
    rtvalue["other_id"] = other_id;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_CREATE_PRIVATE_CHAT_RSP);
    });

    int  thread_id = 0;
    bool res =
        MysqlMgr::GetInstance()->CreatePrivateChat(uid, other_id, thread_id);
    if (!res)
    {
        rtvalue["error"] = ErrorCodes::CREATE_CHAT_FAILED;
        return;
    }

    rtvalue["thread_id"] = thread_id;
}