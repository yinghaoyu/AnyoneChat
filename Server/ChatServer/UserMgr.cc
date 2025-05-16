#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "Logger.h"

UserMgr::~UserMgr() { _uid_to_session.clear(); }

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    auto                        iter = _uid_to_session.find(uid);
    if (iter == _uid_to_session.end())
    {
        return nullptr;
    }

    return iter->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    _uid_to_session[uid] = session;
}

void UserMgr::RmvUserSession(int uid, std::string session_id)
{
    {
        std::lock_guard<std::mutex> lock(_session_mtx);
        auto                        iter = _uid_to_session.find(uid);
        if (iter == _uid_to_session.end())
        {
            return;
        }

        auto session_id_ = iter->second->GetSessionId();
        // 不相等说明是其他地方登录了
        if (session_id_ != session_id)
        {
            LOG_ERROR("session removed failure, uid: {}, old_session: {}, new_session: {}", uid, session_id, session_id_);
            return;
        }
        _uid_to_session.erase(uid);
        LOG_INFO("session removed, uid: {}, session : {}", uid, session_id);
    }
}

UserMgr::UserMgr() {}
