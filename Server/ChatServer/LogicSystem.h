#pragma once
#include "Session.h"
#include "Singleton.h"
#include "data.h"

#include <functional>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <map>
#include <thread>
#include <vector>

class ChatServer;
typedef function<void(
    shared_ptr<Session>, const short& msg_id, const string& msg_data)>
    FunCallBack;
class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

    typedef std::shared_ptr<Session> SessionPtr;

  public:
    void Shutdown();
    void PostMsgToQue(shared_ptr<LogicNode> msg);
    void SetServer(std::shared_ptr<ChatServer> pserver);

  private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks();
    void LoginHandler(
        SessionPtr session, const short& msg_id, const string& msg_data);
    void SearchInfo(
        SessionPtr session, const short& msg_id, const string& msg_data);
    void AddFriendApply(
        SessionPtr session, const short& msg_id, const string& msg_data);
    void AuthFriendApply(
        SessionPtr session, const short& msg_id, const string& msg_data);
    void DealChatTextMsg(
        SessionPtr session, const short& msg_id, const string& msg_data);
    void HeartBeatHandler(
        SessionPtr session, const short& msg_id, const string& msg_data);
    bool isPureDigit(const std::string& str);
    void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
    void GetUserByName(std::string name, Json::Value& rtvalue);
    bool GetBaseInfo(
        std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    bool GetFriendApplyInfo(
        int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
    bool GetFriendList(
        int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);
    void GetUserThreadsHandler(
        SessionPtr session, const short& msg_id, const string& msg_data);
    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
        std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore,
        int& nextLastId);
    void CreatePrivateChat(
        SessionPtr session, const short& msg_id, const string& msg_data);
    std::vector<std::thread>           threads_;
    std::vector<shared_ptr<LogicNode>> msg_que_;
    std::mutex                         mutex_;
    std::condition_variable            consume_;
    bool                               stopped_;
    std::map<short, FunCallBack>       fun_callbacks_;
    std::shared_ptr<ChatServer>        server_;
};
