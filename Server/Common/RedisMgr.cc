#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "DistLock.h"
#include "const.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>

RedisMgr::RedisMgr()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    auto  host    = gCfgMgr["Redis"]["Host"];
    auto  port    = gCfgMgr["Redis"]["Port"];
    auto  pwd     = gCfgMgr["Redis"]["Passwd"];
    con_pool_.reset(new RedisPool(host, stoi(port), pwd, 10));
}

RedisMgr::~RedisMgr() {}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("GET %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
        return false;
    }

    value = reply->str;

    std::cout << "Succeed to execute command [ GET " << key << "  ]"
              << std::endl;
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value)
{
    // 执行redis命令行
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("SET %s %s", key.c_str(), value.c_str());

    // 如果返回NULL则说明执行失败
    if (nullptr == reply)
    {
        std::cout << "Execut command [ SET " << key << "  " << value
                  << " ] failure" << std::endl;
        return false;
    }

    // 如果执行失败则释放连接
    if (!(reply->type == REDIS_REPLY_STATUS &&
            (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
    {
        std::cout << "Execut command [ SET " << key << "  " << value
                  << " ] failure" << std::endl;
        return false;
    }

    std::cout << "Execut command [ SET " << key << "  " << value << " ] success"
              << std::endl;
    return true;
}

bool RedisMgr::SetExp(
    const std::string& key, const std::string& value, int expire_seconds)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }

    // 使用SETEX命令，同时设置值和过期时间
    auto reply = connect->cmd(
        "SETEX %s %d %s", key.c_str(), expire_seconds, value.c_str());

    if (NULL == reply)
    {
        std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
                  << " " << value << " ] failure!" << std::endl;
        return false;
    }

    if (!(reply->type == REDIS_REPLY_STATUS &&
            (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
    {
        std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
                  << " " << value << " ] failure!" << std::endl;
        return false;
    }

    std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
              << " " << value << " ] success!" << std::endl;
    return true;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("LPUSH %s %s", key.c_str(), value.c_str());
    if (nullptr == reply)
    {
        std::cout << "Execut command [ LPUSH " << key << "  " << value
                  << " ] failure" << std::endl;

        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0)
    {
        std::cout << "Execut command [ LPUSH " << key << "  " << value
                  << " ] failure" << std::endl;

        return false;
    }

    std::cout << "Execut command [ LPUSH " << key << "  " << value
              << " ] success" << std::endl;

    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("LPOP %s ", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ LPOP " << key << " ] failure"
                  << std::endl;

        return false;
    }

    if (reply->type == REDIS_REPLY_NIL)
    {
        std::cout << "Execut command [ LPOP " << key << " ] failure"
                  << std::endl;

        return false;
    }

    value = reply->str;
    std::cout << "Execut command [ LPOP " << key << " ] success" << std::endl;

    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("RPUSH %s %s", key.c_str(), value.c_str());
    if (nullptr == reply)
    {
        std::cout << "Execut command [ RPUSH " << key << "  " << value
                  << " ] failure" << std::endl;

        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0)
    {
        std::cout << "Execut command [ RPUSH " << key << "  " << value
                  << " ] failure" << std::endl;

        return false;
    }

    std::cout << "Execut command [ RPUSH " << key << "  " << value
              << " ] success" << std::endl;

    return true;
}
bool RedisMgr::RPop(const std::string& key, std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("RPOP %s ", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ RPOP " << key << " ] failure"
                  << std::endl;

        return false;
    }

    if (reply->type == REDIS_REPLY_NIL)
    {
        std::cout << "Execut command [ RPOP " << key << " ] failure"
                  << std::endl;

        return false;
    }
    value = reply->str;
    std::cout << "Execut command [ RPOP " << key << " ] success" << std::endl;

    return true;
}

bool RedisMgr::HSet(
    const std::string& key, const std::string& hkey, const std::string& value)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply =
        connect->cmd("HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  "
                  << value << " ] failure" << std::endl;

        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  "
                  << value << " ] failure" << std::endl;

        return false;
    }

    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  "
              << value << " ] success" << std::endl;

    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return "";
    }

    auto reply = connect->cmd({"HGET", key, hkey});
    if (reply == nullptr)
    {
        std::cout << "Execut command [ HGet " << key << " " << hkey
                  << "  ] failure" << std::endl;

        return "";
    }

    if (reply->type == REDIS_REPLY_NIL)
    {

        std::cout << "Execut command [ HGet " << key << " " << hkey
                  << "  ] failure" << std::endl;

        return "";
    }

    std::string value = reply->str;

    std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success"
              << std::endl;
    return value;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = connect->cmd("HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr)
    {
        std::cerr << "HDEL command failed" << std::endl;
        return false;
    }

    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER)
    {
        success = reply->integer > 0;
    }

    return success;
}

bool RedisMgr::Del(const std::string& key)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = connect->cmd("DEL %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ Del " << key << " ] failure"
                  << std::endl;

        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        std::cout << "Execut command [ Del " << key << " ] failure"
                  << std::endl;

        return false;
    }

    std::cout << "Execut command [ Del " << key << " ] success" << std::endl;

    return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = connect->cmd("exists %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Not Found [ Key " << key << " ] " << std::endl;

        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer == 0)
    {
        std::cout << "Not Found [ Key " << key << " ] " << std::endl;

        return false;
    }
    std::cout << " Found [ Key " << key << " ] exists" << std::endl;

    return true;
}

std::string RedisMgr::acquireLock(
    const std::string& lockName, int lockTimeout, int acquireTimeout)
{

    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return "";
    }

    Defer defer([&connect, this]() {});

    return DistLock::Inst().acquireLock(
        connect, lockName, lockTimeout, acquireTimeout);
}

bool RedisMgr::releaseLock(
    const std::string& lockName, const std::string& identifier)
{
    if (identifier.empty())
    {
        return true;
    }
    auto connect = con_pool_->get();
    if (connect == nullptr)
    {
        return false;
    }

    Defer defer([&connect, this]() {});

    return DistLock::Inst().releaseLock(connect, lockName, identifier);
}

void RedisMgr::IncreaseCount(std::string server_name)
{
    auto lock_key   = LOCK_COUNT;
    auto identifier = RedisMgr::GetInstance()->acquireLock(
        lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    // 利用defer解锁
    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    // 将登录数量增加
    auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int  count  = 0;
    if (!rd_res.empty())
    {
        count = std::stoi(rd_res);
    }

    count++;
    auto count_str = std::to_string(count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);
}

void RedisMgr::DecreaseCount(std::string server_name)
{
    auto lock_key   = LOCK_COUNT;
    auto identifier = RedisMgr::GetInstance()->acquireLock(
        lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    // 利用defer解锁
    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    // 将登录数量减少
    auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int  count  = 0;
    if (!rd_res.empty())
    {
        count = std::stoi(rd_res);
        if (count > 0)
        {
            count--;
        }
    }

    auto count_str = std::to_string(count);
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);
}

void RedisMgr::InitCount(std::string server_name)
{
    auto lock_key   = LOCK_COUNT;
    auto identifier = RedisMgr::GetInstance()->acquireLock(
        lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    // 利用defer解锁
    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
}

void RedisMgr::DelCount(std::string server_name)
{
    auto lock_key   = LOCK_COUNT;
    auto identifier = RedisMgr::GetInstance()->acquireLock(
        lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    // 利用defer解锁
    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
    });

    RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
}

bool RedisMgr::SetFileInfo(
    const std::string& name, std::shared_ptr<FileInfo> file_info)
{
    Json::Reader reader;
    Json::Value  root;
    root["file_path_str"] = file_info->_file_path_str;
    root["name"]          = file_info->_name;
    root["seq"]           = file_info->_seq;
    root["total_size"]    = file_info->_total_size;
    root["trans_size"]    = file_info->_trans_size;
    auto file_info_str    = root.toStyledString();
    auto redis_key        = "file_upload_" + name;
    bool success          = SetExp(redis_key, file_info_str, 3600);
    return success;
}

bool RedisMgr::SetDownLoadInfo(
    const std::string& name, std::shared_ptr<FileInfo> file_info)
{
    Json::Reader reader;
    Json::Value  root;
    root["file_path_str"] = file_info->_file_path_str;
    root["name"]          = file_info->_name;
    root["seq"]           = file_info->_seq;
    root["total_size"]    = file_info->_total_size;
    root["trans_size"]    = file_info->_trans_size;
    auto file_info_str    = root.toStyledString();
    auto redis_key        = "file_download_" + name;
    bool success          = SetExp(redis_key, file_info_str, 3600);
    return success;
}

bool RedisMgr::DelDownLoadInfo(const std::string& name)
{
    auto redis_key = "file_download_" + name;
    return Del(redis_key);
}

std::shared_ptr<FileInfo> RedisMgr::GetFileInfo(const std::string& name)
{
    auto        redis_key     = "file_upload_" + name;
    std::string file_info_str = "";

    // �� Redis ��ȡ����
    bool success = Get(redis_key, file_info_str);
    if (!success || file_info_str.empty())
    {
        return nullptr;
    }

    // ���� JSON
    Json::Reader reader;
    Json::Value  root;
    if (!reader.parse(file_info_str, root))
    {
        std::cout << "Failed to parse file info JSON for name: " << name
                  << std::endl;
        return nullptr;
    }

    // ���� FileInfo �����������
    auto file_info = std::make_shared<FileInfo>();
    try
    {
        file_info->_file_path_str = root["file_path_str"].asString();
        file_info->_name          = root["name"].asString();
        file_info->_seq           = root["seq"].asInt();
        file_info->_total_size    = root["total_size"].asInt();
        file_info->_trans_size    = root["trans_size"].asInt();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error parsing file info fields for name " << name << ": "
                  << e.what() << std::endl;
        return nullptr;
    }

    return file_info;
}

std::shared_ptr<FileInfo> RedisMgr::GetDownloadInfo(const std::string& name)
{
    auto        redis_key     = "file_download_" + name;
    std::string file_info_str = "";

    // 从 Redis 获取数据
    bool success = Get(redis_key, file_info_str);
    if (!success || file_info_str.empty())
    {
        return nullptr;
    }

    // 解析 JSON
    Json::Reader reader;
    Json::Value  root;
    if (!reader.parse(file_info_str, root))
    {
        std::cout << "Failed to parse file info JSON for name: " << name
                  << std::endl;
        return nullptr;
    }

    // 创建 FileInfo 对象并填充数据
    auto file_info = std::make_shared<FileInfo>();
    try
    {
        file_info->_file_path_str = root["file_path_str"].asString();
        file_info->_name          = root["name"].asString();
        file_info->_seq           = root["seq"].asInt();
        file_info->_total_size    = root["total_size"].asInt();
        file_info->_trans_size    = root["trans_size"].asInt();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error parsing file info fields for name " << name << ": "
                  << e.what() << std::endl;
        return nullptr;
    }

    return file_info;
}