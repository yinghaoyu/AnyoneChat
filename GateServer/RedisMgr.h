#pragma once

#include "const.h"
#include "Singleton.h"
#include "redis.h"

class RedisMgr : public Singleton<RedisMgr>,
                 public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;

  public:
    ~RedisMgr();
    bool        Get(const std::string& key, std::string& value);
    bool        Set(const std::string& key, const std::string& value);
    bool        LPush(const std::string& key, const std::string& value);
    bool        LPop(const std::string& key, std::string& value);
    bool        RPush(const std::string& key, const std::string& value);
    bool        RPop(const std::string& key, std::string& value);
    bool        HSet(const std::string& key, const std::string& hkey,
               const std::string& value);
    bool        HSet(const char* key, const char* hkey, const char* hvalue,
               size_t hvaluelen);
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

  private:
    RedisMgr();
    std::unique_ptr<RedisPool> _con_pool;
};
