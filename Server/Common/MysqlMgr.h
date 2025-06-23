#pragma once
#include "MysqlDao.h"
#include "Singleton.h"
#include "data.h"

#include <vector>

class MysqlMgr : public Singleton<MysqlMgr>
{
    friend class Singleton<MysqlMgr>;

  public:
    ~MysqlMgr();
    int  RegUser(const std::string& name, const std::string& email,
         const std::string& pwd, const std::string& icon);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(
        const std::string& email, const std::string& pwd, UserInfo& userInfo);
    bool AddFriendApply(const int from, const int to);
    bool AuthFriendApply(const int from, const int to);
    bool AddFriend(const int from, const int to, const std::string& back_name);
    std::shared_ptr<UserInfo> GetUser(const int uid);
    std::shared_ptr<UserInfo> GetUser(const std::string& name);

    bool GetApplyList(const int                  touid,
        std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin,
        int limit = 10);
    bool GetFriendList(
        const int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info);
    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
        std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore,
        int& nextLastId);

    bool CreatePrivateChat(int user1_id, int user2_id, int& thread_id);

  private:
    MysqlMgr();
    MysqlDao dao_;
};
