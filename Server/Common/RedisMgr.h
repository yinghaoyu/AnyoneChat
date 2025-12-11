#pragma once

#include "Singleton.h"
#include "redis.h"
#include "FileInfo.h"

class RedisMgr : public Singleton<RedisMgr>,
                 public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;

  public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool SetExp(
        const std::string& key, const std::string& value, int expire_seconds);
    bool        LPush(const std::string& key, const std::string& value);
    bool        LPop(const std::string& key, std::string& value);
    bool        RPush(const std::string& key, const std::string& value);
    bool        RPop(const std::string& key, std::string& value);
    bool        HSet(const std::string& key, const std::string& hkey,
               const std::string& value);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool        HDel(const std::string& key, const std::string& field);
    bool        Del(const std::string& key);
    bool        ExistsKey(const std::string& key);

    std::string acquireLock(
        const std::string& lockName, int lockTimeout, int acquireTimeout);

    bool releaseLock(
        const std::string& lockName, const std::string& identifier);

    void IncreaseCount(std::string server_name);
    void DecreaseCount(std::string server_name);
    void InitCount(std::string server_name);
    void DelCount(std::string server_name);
    bool SetFileInfo(const std::string& name, std::shared_ptr<FileInfo>);
    std::shared_ptr<FileInfo> GetFileInfo(const std::string& name);
    std::shared_ptr<FileInfo> GetDownloadInfo(const std::string& name);
    bool SetDownLoadInfo(const std::string& name, std::shared_ptr<FileInfo>);
    bool DelDownLoadInfo(const std::string& name);

  private:
    RedisMgr();
    std::unique_ptr<RedisPool> con_pool_;
};
