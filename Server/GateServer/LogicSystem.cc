#include "LogicSystem.h"
#include "HttpConnection.h"
#include "Logger.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "StatusGrpcClient.h"
#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>

LogicSystem::LogicSystem()
{
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->response_.body())
            << "receive get_test req " << std::endl;
        int i = 0;
        for (auto& elem : connection->params_)
        {
            i++;
            beast::ostream(connection->response_.body())
                << "param" << i << " key is " << elem.first;
            beast::ostream(connection->response_.body())
                << ", " << " value is " << elem.second << std::endl;
        }

        connection->response_.set(http::field::content_type, "text/plain");
    });

    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str =
            boost::beast::buffers_to_string(connection->request_.body().data());
        LOG_INFO("Request target:{} receive: {}", connection->request_.target(),
                 body_str);
        connection->response_.set(http::field::content_type, "text/json");
        Json::Value  root;
        Json::Reader reader;
        Json::Value  src_root;
        bool         parse_success = reader.parse(body_str, src_root);
        if (!parse_success)
        {
            LOG_ERROR("Failed to parse JSON data");
            root["error"]       = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        if (!src_root.isMember("email"))
        {
            LOG_ERROR("Failed to parse JSON data");
            root["error"]       = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        auto         email = src_root["email"].asString();
        GetVarifyRsp rsp =
            VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        cout << "email is " << email << endl;
        root["error"]       = rsp.error();
        root["email"]       = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->response_.body()) << jsonstr;
        return true;
    });

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str =
            boost::beast::buffers_to_string(connection->request_.body().data());
        LOG_INFO("Request target:{} receive: {}", connection->request_.target(),
                 body_str);
        connection->response_.set(http::field::content_type, "text/json");
        Json::Value  root;
        Json::Reader reader;
        Json::Value  src_root;
        bool         parse_success = reader.parse(body_str, src_root);
        if (!parse_success)
        {
            LOG_ERROR("Failed to parse JSON data");
            root["error"]       = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        auto email   = src_root["email"].asString();
        auto name    = src_root["user"].asString();
        auto pwd     = src_root["passwd"].asString();
        auto confirm = src_root["confirm"].asString();
        auto icon    = src_root["icon"].asString();

        if (pwd != confirm)
        {
            LOG_ERROR("password not match");
            root["error"]       = ErrorCodes::PasswdErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        // 先查找redis中email对应的验证码是否合理
        std::string varify_code;
        bool        b_get_varify = RedisMgr::GetInstance()->Get(
            CODEPREFIX + src_root["email"].asString(), varify_code);
        if (!b_get_varify)
        {
            LOG_INFO("get varify code expired");
            root["error"]       = ErrorCodes::VarifyExpired;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        if (varify_code != src_root["varifycode"].asString())
        {
            LOG_INFO("varify code error");
            root["error"]       = ErrorCodes::VarifyCodeErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        // 查找数据库判断用户是否存在
        int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
        if (uid == 0 || uid == -1)
        {
            LOG_INFO("user or email exist");
            root["error"]       = ErrorCodes::UserExist;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }
        root["error"]       = 0;
        root["uid"]         = uid;
        root["email"]       = email;
        root["user"]        = name;
        root["passwd"]      = pwd;
        root["confirm"]     = confirm;
        root["icon"]        = icon;
        root["varifycode"]  = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->response_.body()) << jsonstr;
        return true;
    });

    // 重置回调逻辑
    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str =
            boost::beast::buffers_to_string(connection->request_.body().data());
        LOG_INFO("Request target:{} receive: {}", connection->request_.target(),
                 body_str);
        connection->response_.set(http::field::content_type, "text/json");
        Json::Value  root;
        Json::Reader reader;
        Json::Value  src_root;
        bool         parse_success = reader.parse(body_str, src_root);
        if (!parse_success)
        {
            LOG_ERROR("Failed to parse JSON data");
            root["error"]       = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        auto email = src_root["email"].asString();
        auto name  = src_root["user"].asString();
        auto pwd   = src_root["passwd"].asString();

        // 先查找redis中email对应的验证码是否合理
        std::string varify_code;
        bool        b_get_varify = RedisMgr::GetInstance()->Get(
            CODEPREFIX + src_root["email"].asString(), varify_code);
        if (!b_get_varify)
        {
            LOG_INFO("get varify code expired");
            root["error"]       = ErrorCodes::VarifyExpired;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        if (varify_code != src_root["varifycode"].asString())
        {
            LOG_INFO("varify code error");
            root["error"]       = ErrorCodes::VarifyCodeErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }
        // 查询数据库判断用户名和邮箱是否匹配
        bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (!email_valid)
        {
            LOG_INFO("user email not match");
            root["error"]       = ErrorCodes::EmailNotMatch;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        // 更新密码为最新密码
        bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
        if (!b_up)
        {
            LOG_INFO("update password failed");
            root["error"]       = ErrorCodes::PasswdUpFailed;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }
        LOG_INFO("update password succeed");
        root["error"]       = 0;
        root["email"]       = email;
        root["user"]        = name;
        root["passwd"]      = pwd;
        root["varifycode"]  = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->response_.body()) << jsonstr;
        return true;
    });

    // 用户登录逻辑
    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str =
            boost::beast::buffers_to_string(connection->request_.body().data());
        LOG_INFO("Request target:{} receive: {}", connection->request_.target(),
                 body_str);
        connection->response_.set(http::field::content_type, "text/json");
        Json::Value  root;
        Json::Reader reader;
        Json::Value  src_root;
        bool         parse_success = reader.parse(body_str, src_root);
        if (!parse_success)
        {
            LOG_ERROR("Failed to parse JSON data");
            root["error"]       = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        auto     email = src_root["email"].asString();
        auto     pwd   = src_root["passwd"].asString();
        UserInfo userInfo;
        // 查询数据库判断用户名和密码是否匹配
        bool pwd_valid =
            MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
        if (!pwd_valid)
        {
            LOG_INFO("user email not match");
            root["error"]       = ErrorCodes::PasswdInvalid;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }

        LOG_INFO("gRpc get chat server begin uid: {}", userInfo.uid_);
        // 查询StatusServer找到合适的连接
        auto reply =
            StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid_);
        if (reply.error())
        {
            LOG_ERROR("get chat server failed error: {}", reply.error());
            root["error"]       = ErrorCodes::RPCFailed;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body()) << jsonstr;
            return true;
        }
        LOG_INFO("gRPC get chat server success uid: {}", userInfo.uid_);

        root["error"]        = 0;
        root["email"]        = email;
        root["uid"]          = userInfo.uid_;
        root["token"]        = reply.token();
        root["chathost"]     = reply.host();
        root["chatport"]     = reply.port();
        auto&       gCfgMgr  = ConfigMgr::Inst();
        std::string res_port = gCfgMgr["ResServer"]["Port"];
        std::string res_host = gCfgMgr["ResServer"]["Host"];
        root["reshost"]      = res_host;
        root["resport"]      = res_port;
        std::string jsonstr  = root.toStyledString();
        beast::ostream(connection->response_.body()) << jsonstr;
        return true;
    });
}

void LogicSystem::RegGet(const std::string& url, HttpHandler handler)
{
    m_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(const std::string& url, HttpHandler handler)
{
    m_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::~LogicSystem() {}

bool LogicSystem::HandleGet(
    const std::string& path, std::shared_ptr<HttpConnection> con)
{
    if (!m_get_handlers.count(path))
    {
        return false;
    }

    m_get_handlers[path](con);
    return true;
}

bool LogicSystem::HandlePost(
    const std::string& path, std::shared_ptr<HttpConnection> con)
{
    if (!m_post_handlers.count(path))
    {
        return false;
    }

    m_post_handlers[path](con);
    return true;
}
