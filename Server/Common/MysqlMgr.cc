#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {}

int MysqlMgr::RegUser(const std::string& name, const std::string& email,
    const std::string& pwd, const std::string& icon)
{
    return dao_.RegUser(name, email, pwd, icon);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email)
{
    return dao_.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    return dao_.UpdatePwd(name, newpwd);
}

MysqlMgr::MysqlMgr() {}

bool MysqlMgr::CheckPwd(
    const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    return dao_.CheckPwd(email, pwd, userInfo);
}

bool MysqlMgr::AddFriendApply(const int from, const int to)
{
    return dao_.AddFriendApply(from, to);
}

bool MysqlMgr::AuthFriendApply(const int from, const int to)
{
    return dao_.AuthFriendApply(from, to);
}

bool MysqlMgr::AddFriend(
    const int from, const int to, const std::string& back_name)
{
    return dao_.AddFriend(from, to, back_name);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
    return dao_.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(const std::string& name)
{
    return dao_.GetUser(name);
}

bool MysqlMgr::GetApplyList(const int        touid,
    std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{

    return dao_.GetApplyList(touid, applyList, begin, limit);
}

bool MysqlMgr::GetFriendList(
    const int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info)
{
    return dao_.GetFriendList(self_id, user_info);
}

bool MysqlMgr::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
    std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore,
    int& nextLastId)
{
    return dao_.GetUserThreads(
        userId, lastId, pageSize, threads, loadMore, nextLastId);
}

bool MysqlMgr::CreatePrivateChat(int user1_id, int user2_id, int& thread_id)
{
    return dao_.CreatePrivateChat(user1_id, user2_id, thread_id);
}