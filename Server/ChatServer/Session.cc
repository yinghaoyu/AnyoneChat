#include "Session.h"
#include "ChatServer.h"
#include "Logger.h"
#include "LogicSystem.h"
#include "RedisMgr.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>

Session::Session(boost::asio::io_context& io_context, ChatServer* server)
    : socket_(io_context),
      server_(server),
      closed_(false),
      head_parse_(false),
      user_uid_(0)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    session_id_               = boost::uuids::to_string(a_uuid);
    recv_head_node_           = make_shared<MsgNode>(HEAD_TOTAL_LEN);
    last_heartbeat_           = std::time(nullptr);
}
Session::~Session() { LOG_TRACE("Session dtor~"); }

tcp::socket& Session::GetSocket() { return socket_; }

const std::string& Session::GetSessionId() { return session_id_; }

void Session::SetUserId(int uid) { user_uid_ = uid; }

int Session::GetUserId() { return user_uid_; }

void Session::Start() { AsyncReadHead(HEAD_TOTAL_LEN); }

void Session::Send(const std::string& msg, const short msgid)
{
    Send(msg.c_str(), msg.length(), msgid);
}

void Session::Send(const char* msg, const short max_length, const short msgid)
{
    std::lock_guard<std::mutex> _(send_mutex_);

    int send_que_size = send_que_.size();
    if (send_que_size > MAX_SENDQUE)
    {
        LOG_ERROR("session: {} send que fulled, size: {}", session_id_,
                  MAX_SENDQUE);
        return;
    }

    send_que_.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0)
    {
        return;
    }
    auto& msgnode = send_que_.front();
    boost::asio::async_write(socket_,
        boost::asio::buffer(msgnode->data_, msgnode->total_len_),
        std::bind(
            &Session::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void Session::ShutDownWrite() { socket_.shutdown(tcp::socket::shutdown_send); }

void Session::Close()
{
    std::lock_guard<std::mutex> _(session_mutex_);

    if (closed_)
    {
        return;
    }

    socket_.close();

    closed_ = true;
}

std::shared_ptr<Session> Session::SharedSelf() { return shared_from_this(); }

void Session::AsyncReadBody(int total_len)
{
    auto self = shared_from_this();
    asyncReadFull(total_len,
        [self, this, total_len](
            const boost::system::error_code& ec, std::size_t bytes_transfered) {
            try
            {
                if (ec)
                {
                    LOG_ERROR("session: {} handle read failed, error: {}",
                          session_id_, ec.message());
                    Close();
                    DealExceptionSession();
                    return;
                }

                if (bytes_transfered < total_len)
                {
                    LOG_ERROR(
                    "session: {} read length not match, read: {} , total: {}",
                    session_id_, bytes_transfered, total_len);

                    Close();
                    server_->CleanSession(session_id_);
                    return;
                }

                // 判断连接无效
                if (!server_->CheckValid(session_id_))
                {
                    ShutDownWrite();
                    return;
                }

                memcpy(recv_msg_node_->data_, data_, bytes_transfered);
                recv_msg_node_->cur_len_ += bytes_transfered;
                recv_msg_node_->data_[recv_msg_node_->total_len_] = '\0';
                LOG_INFO("session: {} recv msg data: {}", session_id_,
                     recv_msg_node_->data_);
                // 更新session心跳时间
                UpdateHeartbeat();
                // 此处将消息投递到逻辑队列中
                LogicSystem::GetInstance()->PostMsgToQue(
                    make_shared<LogicNode>(shared_from_this(), recv_msg_node_));
                // 继续监听头部接受事件
                AsyncReadHead(HEAD_TOTAL_LEN);
            }
            catch (std::exception& e)
            {
                LOG_ERROR("session: {} handle read exception, error: {}",
                      session_id_, e.what());
            }
        });
}

void Session::AsyncReadHead(int total_len)
{
    auto self = shared_from_this();
    asyncReadFull(HEAD_TOTAL_LEN,
        [self, this](
            const boost::system::error_code& ec, std::size_t bytes_transfered) {
            try
            {
                if (ec)
                {
                    LOG_ERROR("session: {} handle read failed, error: {}",
                          session_id_, ec.message());
                    Close();
                    DealExceptionSession();
                    return;
                }

                if (bytes_transfered < HEAD_TOTAL_LEN)
                {
                    LOG_ERROR(
                    "session: {} read length not match, read: {} , total: {}",
                    session_id_, bytes_transfered, HEAD_TOTAL_LEN);
                    Close();
                    server_->CleanSession(session_id_);
                    return;
                }

                // 判断连接无效
                if (!server_->CheckValid(session_id_))
                {
                    LOG_ERROR("session: {} check valid failed", session_id_);
                    ShutDownWrite();
                    return;
                }

                recv_head_node_->Clear();
                memcpy(recv_head_node_->data_, data_, bytes_transfered);

                // 获取头部MSGID数据
                short msg_id = 0;
                memcpy(&msg_id, recv_head_node_->data_, HEAD_ID_LEN);
                // 网络字节序转化为本地字节序
                msg_id = boost::asio::detail::socket_ops::network_to_host_short(
                    msg_id);
                LOG_TRACE("session: {} msg_id: {}", session_id_, msg_id);
                // id非法
                if (msg_id > MAX_LENGTH)
                {
                    LOG_ERROR("session: {} msg_id invalid, msg_id: {}", session_id_,
                          msg_id);
                    server_->CleanSession(session_id_);
                    return;
                }
                short msg_len = 0;
                memcpy(&msg_len,
                    recv_head_node_->data_ + HEAD_ID_LEN,
                    HEAD_DATA_LEN);
                // 网络字节序转化为本地字节序
                msg_len =
                    boost::asio::detail::socket_ops::network_to_host_short(
                        msg_len);

                LOG_TRACE("session: {} msg_len: {}", session_id_, msg_len);

                // id非法
                if (msg_len > MAX_LENGTH)
                {
                    LOG_ERROR("session: {} msg_id length, length: {}", session_id_,
                          msg_len);
                    server_->CleanSession(session_id_);
                    return;
                }

                recv_msg_node_ = make_shared<RecvNode>(msg_len, msg_id);
                AsyncReadBody(msg_len);
            }
            catch (std::exception& e)
            {
                LOG_ERROR("session: {} handle read exception, error: {}",
                      session_id_, e.what());
            }
        });
}

void Session::HandleWrite(const boost::system::error_code& error,
    std::shared_ptr<Session>                               shared_self)
{
    try
    {
        if (error)
        {
            LOG_ERROR("session: {} handle write failed, error: {}", session_id_,
                      error.message());
            Close();
            DealExceptionSession();
        }

        auto self = shared_from_this();

        std::lock_guard<std::mutex> lock(send_mutex_);

        send_que_.pop();
        if (!send_que_.empty())
        {
            auto& msgnode = send_que_.front();
            boost::asio::async_write(socket_,
                boost::asio::buffer(msgnode->data_, msgnode->total_len_),
                std::bind(&Session::HandleWrite,
                    this,
                    std::placeholders::_1,
                    shared_self));
        }
    }
    catch (std::exception& e)
    {
        LOG_ERROR("session: {} handle write exception, error: {}", session_id_,
                  e.what());
    }
}

// 读取完整长度
void Session::asyncReadFull(std::size_t maxLength,
    std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
    ::memset(data_, 0, MAX_LENGTH);
    asyncReadLen(0, maxLength, handler);
}

// 读取指定字节数
void Session::asyncReadLen(std::size_t read_len, std::size_t total_len,
    std::function<void(const boost::system::error_code&, std::size_t)> handler)
{
    auto self = shared_from_this();
    socket_.async_read_some(
        boost::asio::buffer(data_ + read_len, total_len - read_len),
        [read_len, total_len, handler, self](
            const boost::system::error_code& ec, std::size_t bytesTransfered) {
            if (ec)
            {
                // 出现错误，调用回调函数
                handler(ec, read_len + bytesTransfered);
                return;
            }

            if (read_len + bytesTransfered >= total_len)
            {
                // 长度够了就调用回调函数
                handler(ec, read_len + bytesTransfered);
                return;
            }

            // 没有错误，且长度不足则继续读取
            self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
        });
}

void Session::NotifyOffline(int uid)
{

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"]   = uid;

    std::string return_str = rtvalue.toStyledString();
    LOG_INFO("session: {} notify offline, uid: {}", session_id_, uid);
    Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
    return;
}

LogicNode::LogicNode(shared_ptr<Session> session, shared_ptr<RecvNode> recvnode)
    : session_(session), recv_node_(recvnode)
{}

bool Session::IsHeartbeatExpired(std::time_t& now)
{
    double diff_sec = std::difftime(now, last_heartbeat_);
    if (diff_sec > 20)
    {
        LOG_INFO("session: {} heartbeat expired, diff sec: {}", session_id_,
                 diff_sec);
        return true;
    }

    return false;
}

void Session::UpdateHeartbeat() { last_heartbeat_ = std::time(nullptr); }

void Session::DealExceptionSession()
{
    auto self = shared_from_this();
    // 加锁清除session
    auto uid_str  = std::to_string(user_uid_);
    auto lock_key = LOCK_PREFIX + uid_str;

    LOG_INFO("Redis get user lock begin, session: {}, key: {}", session_id_,
             lock_key);

    auto identifier = RedisMgr::GetInstance()->acquireLock(
        lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

    LOG_INFO("Redis get user lock finish, session: {}, key: {}", session_id_,
             lock_key);

    Defer defer([identifier, lock_key, self, this]() {
        server_->CleanSession(session_id_);
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    if (identifier.empty())
    {
        LOG_INFO("Redis get redis user lock failed, session: {}, key: {}",
                 session_id_, lock_key);
        return;
    }
    std::string redis_session_id = "";
    auto        bsuccess         = RedisMgr::GetInstance()->Get(
        USER_SESSION_PREFIX + uid_str, redis_session_id);
    if (!bsuccess)
    {
        LOG_ERROR("Redis get session id failed, session: {}, key: {}",
                  session_id_, USER_SESSION_PREFIX + uid_str);
        return;
    }

    if (redis_session_id != session_id_)
    {
        // 说明有客户在其他服务器异地登录了
        LOG_INFO("new session established, uid: {}, old session: {}, "
                 "redis_session: {}",
                 user_uid_, session_id_, redis_session_id);
        return;
    }

    LOG_INFO("Redis clear user login trace, uid: {}", user_uid_);

    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    // 清除用户登录信息
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    // 清除用户token信息
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Del(token_key);
}
