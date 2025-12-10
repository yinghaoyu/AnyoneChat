#include "MysqlDao.h"
#include "ConfigMgr.h"
#include "Logger.h"

MysqlDao::MysqlDao()
{
    auto&       cfg    = ConfigMgr::Inst();
    const auto& host   = cfg["Mysql"]["Host"];
    const auto& port   = cfg["Mysql"]["Port"];
    const auto& pwd    = cfg["Mysql"]["Passwd"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user   = cfg["Mysql"]["User"];
    pool_.reset(new MySQLPool(host, stoi(port), user, pwd, schema, 5, 5));
}

MysqlDao::~MysqlDao() {}

int MysqlDao::RegUser(const std::string& name, const std::string& email,
    const std::string& pwd, const std::string& icon)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return -1;
    }

    try
    {
        // 开始事务
        MySQLTransaction::ptr tran = std::static_pointer_cast<MySQLTransaction>(
            con->openTransaction(false));
        if (!tran->begin())
        {
            std::cerr << "Failed to begin transaction" << std::endl;
            return -1;
        }

        // 检查 email 是否已存在
        auto emailCheck = tran->getMySQL()->queryStmt(
            "SELECT 1 FROM user WHERE email = ?", email.c_str());

        if (emailCheck && emailCheck->next())
        {
            tran->rollback();
            std::cerr << "Email " << email << " already exists" << std::endl;
            return 0;
        }

        // 检查 name 是否已存在
        auto nameCheck = tran->getMySQL()->queryStmt(
            "SELECT 1 FROM user WHERE name = ?", name.c_str());
        if (nameCheck && nameCheck->next())
        {
            tran->rollback();
            std::cerr << "Name " << name << " already exists" << std::endl;
            return 0;
        }

        // 更新 user_id 表
        if (tran->execute("UPDATE user_id SET id = id + 1") != 0)
        {
            tran->rollback();
            std::cerr << "Failed to update user_id" << std::endl;
            return -1;
        }

        // 获取新的用户 ID
        auto userIdResult =
            tran->getMySQL()->queryStmt("SELECT id FROM user_id");
        int newId = 0;
        if (userIdResult && userIdResult->next())
        {
            newId = userIdResult->getInt32(0);
        }
        else
        {
            tran->rollback();
            std::cerr << "Failed to retrieve new user ID" << std::endl;
            return -1;
        }

        // 插入新用户
        if (tran->getMySQL()->execStmt(
                "INSERT INTO user (uid, name, email, pwd, nick, icon) VALUES "
                "(?, ?, ?, ?, ?, ?)",
                newId,
                name.c_str(),
                email.c_str(),
                pwd.c_str(),
                name.c_str(),
                icon.c_str()) != 0)
        {
            tran->rollback();
            std::cerr << "Failed to insert new user" << std::endl;
            return -1;
        }

        // 提交事务
        if (!tran->commit())
        {
            std::cerr << "Failed to commit transaction" << std::endl;
            return -1;
        }

        std::cout << "User registered successfully with ID: " << newId
                  << std::endl;
        return newId;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in RegUser: {}", e.what());
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        auto result = con->queryStmt(
            "SELECT email FROM user WHERE name = ?", name.c_str());
        if (result && result->next())
        {
            std::string fetchedEmail = result->getString(0);
            return fetchedEmail == email;
        }
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in CheckEmail: {}", e.what());
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        int ret = con->execStmt("UPDATE user SET pwd = ? WHERE name = ?",
            newpwd.c_str(),
            name.c_str());
        if (ret == 0)
        {
            LOG_DEBUG("Password updated successfully for user: {}", name);
            return true;
        }
        else
        {
            LOG_ERROR("No rows affected for user: {}", name);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UpdatePwd: ", e.what());
        return false;
    }
}

bool MysqlDao::CheckPwd(
    const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        auto result = con->queryStmt(
            "SELECT uid, name, email, pwd FROM user WHERE email = ?",
            email.c_str());
        if (result && result->next())
        {
            std::string origin_pwd = result->getString(3);
            if (pwd != origin_pwd)
            {
                return false;
            }

            // 填充 userInfo
            userInfo.uid_   = result->getInt32(0);
            userInfo.name_  = result->getString(1);
            userInfo.email_ = result->getString(2);
            userInfo.pwd_   = origin_pwd;

            return true;
        }
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in CheckPwd: {}", e.what());
        return false;
    }
}

bool MysqlDao::AddFriendApply(const int from, const int to,
    const std::string& desc, const std::string& back_name)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        int ret = con->execStmt(
            "INSERT INTO friend_apply (from_uid, to_uid, descs, back_name) "
            "VALUES (?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE from_uid = VALUES(from_uid), to_uid = "
            "VALUES(to_uid),  descs = ?, back_name = ?",
            (int32_t)from,
            (int32_t)to,
            desc.c_str(),
            back_name.c_str());

        if (ret == 0)
        {
            LOG_DEBUG("Friend apply added or updated successfully, fromuid: "
                      "{}, touid: {}",
                      from, to);
            return true;
        }
        else
        {
            LOG_ERROR(
                "No rows affected for friend apply, fromuid: {}, touid: {} ",
                from, to);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in AddFriendApply: {}", e.what());
        return false;
    }
}

bool MysqlDao::AuthFriendApply(const int from, const int to)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        // 反过来的申请时from，验证时to
        int ret = con->execStmt("UPDATE friend_apply SET status = 1 "
                                "WHERE from_uid = ? AND to_uid = ?",
            (int32_t)to,
            (int32_t)from);

        if (ret == 0)
        {
            LOG_DEBUG("Friend apply authenticated successfully, fromuid: {}, "
                      "touid: {}",
                      from, to);
            return true;
        }
        else
        {
            LOG_ERROR("No rows affected for friend apply authentication, "
                      "fromuid: {}, touid: {}",
                      from, to);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in AuthFriendApply: {}", e.what());
        return false;
    }
}

bool MysqlDao::AddFriend(const int from, const int to,
    const std::string&                          back_name,
    std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        // 开始事务
        MySQLTransaction::ptr tran = std::static_pointer_cast<MySQLTransaction>(
            con->openTransaction(false));
        if (!tran->begin())
        {
            LOG_ERROR("Failed to begin transaction");
            return false;
        }

        std::string reverse_back;
        std::string apply_desc;

        // 1. 锁定并读取
        {
            auto result =
                tran->getMySQL()->queryStmt("SELECT back_name, descs "
                                            "FROM friend_apply "
                                            "WHERE from_uid = ? AND to_uid = ? "
                                            "FOR UPDATE",
                    (int32_t)to,
                    (int32_t)from);

            if (result && result->next())
            {
                reverse_back = result->getString(0);
                apply_desc   = result->getString(1);
            }
            else
            {
                tran->rollback();
                LOG_ERROR("No friend apply found for fromuid: {}, touid: {}", to, from);
                return false;
            }
        }

        // 2.执行真正的更新
        {
            int ret =
                tran->getMySQL()->execStmt("UPDATE friend_apply "
                                           "SET status = 1 "
                                           "WHERE from_uid = ? AND to_uid = ?",
                    (int32_t)to,
                    (int32_t)from);
            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to update friend_apply for fromuid: {}, touid: {}", to, from);
                return false;
            }
        }

        // 假如 A(1001) 和 B(1002) 同时互相加好友，
        // 那么事务A分别插入数据 (1001, 1002)、(1002, 1001)
        // 那么事务B分别插入数据 (1002, 1001)、(1001, 1002)
        // 这种情况显然会发生死锁，而数据库自带的死锁检测通常耗时较长
        // 死锁检测通常伴随着事务回滚，为防止两个事务发生死锁
        // 可以让A和B都以(1001, 1002)、(1002, 1001)相同顺序插入
        int min = std::min(from, to);
        int max = std::max(from, to);
        // 3. 插入认证方好友数据
        {
            int         x   = from == min ? from : to;
            int         y   = from == min ? to : from;
            std::string str = from == min ? back_name : reverse_back;

            int ret = tran->getMySQL()->execStmt(
                "INSERT IGNORE INTO friend (self_id, "
                "friend_id, back) VALUES (?, ?, ?)",
                (int32_t)x,
                (int32_t)y,
                str);
            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert friend for user: {}", from);
                return false;
            }
        }

        // 4. 插入申请方好友数据
        {
            int         x   = from == max ? from : to;
            int         y   = from == max ? to : from;
            std::string str = from == max ? back_name : reverse_back;

            int ret = tran->getMySQL()->execStmt(
                "INSERT IGNORE INTO friend (self_id, "
                "friend_id, back) VALUES (?, ?, ?)",
                (int32_t)x,
                (int32_t)y,
                str);
            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert friend for user: {}", to);
                return false;
            }
        }

        // 5. 创建 chat_thread 记录
        {
            int ret = tran->getMySQL()->execStmt(
                "INSERT INTO chat_thread (type, created_at) VALUES "
                "('private', NOW());");
            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert friend for user: {}", to);
                return false;
            }
        }
        uint64_t threadId = tran->getMySQL()->getInsertId();

        // 6. 插入 private_chat 记录
        {
            int ret = tran->getMySQL()->execStmt(
                "INSERT INTO private_chat(thread_id, "
                "user1_id, user2_id) VALUES (?, ?, ?)",
                (uint64_t)threadId,
                (uint32_t)from,
                (uint32_t)to);
            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert friend for user: {}", to);
                return false;
            }
        }

        // 7. 插入初始消息到 chat_message 表
        if (!apply_desc.empty())
        {
            int ret = tran->getMySQL()->execStmt(
                "INSERT INTO chat_message(thread_id, sender_id, recv_id, "
                "content,created_at, updated_at, status) VALUES (?, ?, ?, "
                "?,NOW(),NOW(),?)",
                (uint64_t)threadId,
                (uint32_t)to,
                (uint32_t)from,
                apply_desc.c_str(),
                (uint32_t)2);

            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert chat_message for user: {}", to);
                return false;
            }

            auto messageId = tran->getMySQL()->getInsertId();
            auto tx_data   = std::make_shared<AddFriendMsg>();
            tx_data->set_sender_id(to);
            tx_data->set_msg_id(messageId);
            tx_data->set_msgcontent(apply_desc);
            tx_data->set_thread_id(threadId);
            tx_data->set_unique_id("");
            tx_data->set_status(2);
            LOG_INFO("Addfriend insert message success");
            chat_datas.push_back(tx_data);
        }

        // 8. 插入成为好友的消息
        {
            std::string hello_str = "Hello, I'm " + reverse_back;

            int ret = tran->getMySQL()->execStmt(
                "INSERT INTO chat_message(thread_id, sender_id, recv_id, "
                "content, created_at, updated_at, status) VALUES (?, ?, ?, "
                "?,NOW(),NOW(),?)",
                (uint64_t)threadId,
                (uint32_t)from,
                (uint32_t)to,
                hello_str,
                (uint32_t)2);

            if (ret != 0)
            {
                tran->rollback();
                LOG_ERROR("Failed to insert chat_message for user: {}", to);
                return false;
            }

            auto messageId = tran->getMySQL()->getInsertId();
            auto tx_data   = std::make_shared<AddFriendMsg>();
            tx_data->set_sender_id(from);
            tx_data->set_msg_id(messageId);
            tx_data->set_msgcontent(hello_str);
            tx_data->set_thread_id(threadId);
            tx_data->set_unique_id("");
            tx_data->set_status(2);
            chat_datas.push_back(tx_data);
        }

        // 提交事务
        if (!tran->commit())
        {
            LOG_ERROR("Failed to commit transaction");
            return false;
        }

        LOG_DEBUG(
            "Friend relationship added successfully, fromuid: {}, touid: {}",
            from, to);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in AddFriend: {}", e.what());
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(const int uid)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return nullptr;
    }

    try
    {
        // desc为 mysql 关键字
        auto result =
            con->queryStmt("SELECT uid, name, email, pwd, nick, `desc`, sex, "
                           "icon FROM user WHERE uid = ?",
                (int32_t)uid);
        if (result && result->next())
        {
            auto user_ptr    = std::make_shared<UserInfo>();
            user_ptr->uid_   = uid;
            user_ptr->name_  = result->getString(1);
            user_ptr->email_ = result->getString(2);
            user_ptr->pwd_   = result->getString(3);
            user_ptr->nick_  = result->getString(4);
            user_ptr->desc_  = result->getString(5);
            user_ptr->sex_   = result->getInt32(6);
            user_ptr->icon_  = result->getString(7);
            return user_ptr;
        }
        return nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in GetUser: {}", e.what());
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(const std::string& name)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return nullptr;
    }

    try
    {
        auto result =
            con->queryStmt("SELECT uid, name, email, pwd, nick, `desc`, sex, "
                           "icon FROM user WHERE name = ?",
                name.c_str());
        if (result && result->next())
        {
            auto user_ptr    = std::make_shared<UserInfo>();
            user_ptr->uid_   = result->getInt32(0);
            user_ptr->name_  = name;
            user_ptr->email_ = result->getString(2);
            user_ptr->pwd_   = result->getString(3);
            user_ptr->nick_  = result->getString(4);
            user_ptr->desc_  = result->getString(5);
            user_ptr->sex_   = result->getInt32(6);
            user_ptr->icon_  = result->getString(7);
            return user_ptr;
        }
        return nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in GetUser: {}", e.what());
        return nullptr;
    }
}

bool MysqlDao::GetApplyList(const int        touid,
    std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        auto result = con->queryStmt("SELECT apply.from_uid, apply.status, "
                                     "user.name, user.nick, user.sex "
                                     "FROM friend_apply AS apply "
                                     "JOIN user ON apply.from_uid = user.uid "
                                     "WHERE apply.to_uid = ? AND apply.id > ? "
                                     "ORDER BY apply.id ASC LIMIT ?",
            (int32_t)touid,
            begin,
            limit);

        while (result && result->next())
        {
            auto apply_ptr = std::make_shared<ApplyInfo>(result->getInt32(0),
                result->getString(2),
                "",  // email (未查询)
                "",  // pwd (未查询)
                result->getString(3),
                result->getInt32(4),
                result->getInt16(1));

            applyList.push_back(apply_ptr);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in GetApplyList: {}", e.what());
        return false;
    }
}

bool MysqlDao::GetFriendList(
    const int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        auto result =
            con->queryStmt("SELECT friend.friend_id, friend.back, user.name, "
                           "user.email, user.nick, user.sex, user.icon "
                           "FROM friend "
                           "JOIN user ON friend.friend_id = user.uid "
                           "WHERE friend.self_id = ?",
                (int32_t)self_id);

        // 遍历结果集
        while (result && result->next())
        {
            auto user_ptr    = std::make_shared<UserInfo>();
            user_ptr->uid_   = result->getInt32(0);
            user_ptr->name_  = result->getString(2);
            user_ptr->email_ = result->getString(3);
            user_ptr->nick_  = result->getString(4);
            user_ptr->sex_   = result->getInt32(5);
            user_ptr->icon_  = result->getString(6);
            user_ptr->back_  = result->getString(1);

            user_info_list.push_back(user_ptr);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in GetFriendList: {}", e.what());
        return false;
    }
}

bool MysqlDao::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
    std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore,
    int& nextLastId)
{
    // 初始状态
    loadMore   = false;
    nextLastId = lastId;
    threads.clear();

    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        auto result = con->queryStmt(
            "WITH all_threads AS ( "
            "  SELECT thread_id, 'private' AS type, user1_id, user2_id "
            "    FROM private_chat "
            "   WHERE (user1_id = ? OR user2_id = ?) "
            "     AND thread_id > ? "
            "  UNION ALL "
            "  SELECT thread_id, 'group' AS type, CAST(0 AS UNSIGNED) AS "
            "user1_id, CAST(0 AS UNSIGNED) AS user2_id "
            "    FROM group_chat_member "
            "   WHERE user_id   = ? "
            "     AND thread_id > ? "
            ") "
            "SELECT thread_id, type, user1_id, user2_id "
            "  FROM all_threads "
            " ORDER BY thread_id "
            " LIMIT ?;",
            (int64_t)userId,
            (int64_t)userId,
            (int64_t)lastId,
            (int64_t)userId,
            (int64_t)lastId,
            (int32_t)pageSize + 1);

        // 先把所有行读到临时容器
        std::vector<std::shared_ptr<ChatThreadInfo>> tmp;
        // 遍历结果集
        while (result && result->next())
        {
            auto cti        = std::make_shared<ChatThreadInfo>();
            cti->_thread_id = result->getInt64(0);
            cti->_type      = result->getString(1);
            cti->_user1_id  = result->getInt64(2);
            cti->_user2_id  = result->getInt64(3);
            tmp.push_back(cti);
        }
        // 判断是否多取到一条
        if ((int)tmp.size() > pageSize)
        {
            loadMore = true;
            tmp.pop_back();  // 丢掉第 pageSize+1 条
        }

        // 如果还有数据，更新 nextLastId 为最后一条的 thread_id
        if (!tmp.empty())
        {
            nextLastId = tmp.back()->_thread_id;
        }

        // 移入输出向量
        threads = std::move(tmp);

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in GetUserThreads: {}", e.what());
        return false;
    }
}

bool MysqlDao::CreatePrivateChat(int user1_id, int user2_id, int& thread_id)
{

    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        // 开始事务
        MySQLTransaction::ptr tran = std::static_pointer_cast<MySQLTransaction>(
            con->openTransaction(false));
        if (!tran->begin())
        {
            LOG_ERROR("Failed to begin transaction");
            return false;
        }
        // 保证插入的一致性，避免两个进程执行相同插入
        int uid1 = std::min(user1_id, user2_id);
        int uid2 = std::max(user1_id, user2_id);
    again:
        // 如果这里有 SELECT FOR UPDATE，数据不存在时，会产生间隙锁
        // 当事务A(1001)和B(1002)同时进行时，下面 INSERT INTO chat_thread
        // 会产生意向锁，意向锁和间隙锁时互斥的，因此会发生死锁
        // 解决方法是，先插入（乐观策略）
        // 如果有ER_DUP_ENTRY，说明被其他事务插入相同数据
        // 重新查询返回即可
        auto result = tran->getMySQL()->queryStmt(
            "SELECT thread_id FROM private_chat "
            "WHERE (user1_id = ? AND user2_id = ?);",
            (int32_t)uid1,
            (int32_t)uid2);

        if (result && result->next())
        {
            // 1. 如果已存在，返回该 thread_id
            LOG_INFO("Thread_id already exists");
            thread_id = result->getInt64(0);
            tran->commit();
            return true;
        }

        // 2. 如果未找到，创建新的 chat_thread 和 private_chat 记录
        // 在 chat_thread 表插入新记录
        int ret = tran->getMySQL()->execStmt(
            "INSERT INTO chat_thread (type, created_at) VALUES ('private', "
            "NOW());");

        // ER_DUP_ENTRY 1062 重复插入
        if (ret == 1062)
        {
            // FIXME(yinghaoyu): 如果怕多次无效可以限制循环次数
            goto again;
        }

        if (ret != 0)
        {
            LOG_ERROR("Failed to insert chat_thread");
            tran->rollback();
            return false;
        }

        thread_id = tran->getMySQL()->getLastInsertId();

        // 3. 在 private_chat 表插入新记录

        ret = tran->getMySQL()->execStmt(
            "INSERT INTO private_chat (thread_id, user1_id, user2_id, "
            "created_at) "
            "VALUES (?, ?, ?, NOW());",
            (int64_t)thread_id,
            (int64_t)uid1,
            (int64_t)uid2);

        if (ret != 0)
        {
            LOG_ERROR("Failed to insert private_chat");
            tran->rollback();
            return false;
        }

        if (!tran->commit())
        {
            LOG_ERROR("Failed to commit transaction");
            return false;
        }
        LOG_INFO("CreatePrivateChat success uid1: {}, uid2: {}, thread_id: {}", uid1, uid2, thread_id);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in CreatePrivateChat: {}", e.what());
        return false;
    }
}

std::shared_ptr<PageResult> MysqlDao::LoadChatMsg(
    int thread_id, int last_message_id, int page_size)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return nullptr;
    }

    try
    {
        auto page_res         = std::make_shared<PageResult>();
        page_res->load_more   = false;
        page_res->next_cursor = last_message_id;
        uint32_t fetch_limit  = page_size + 1;
        // SQL：多取一条，用于判断是否还有更多
        auto result = con->queryStmt(
            "SELECT message_id, thread_id, sender_id, recv_id, "
            "content, created_at, updated_at, status FROM chat_message WHERE "
            "thread_id = ? AND message_id > ? ORDER BY message_id ASC LIMIT ?",
            (uint64_t)thread_id,
            (uint32_t)last_message_id,
            (uint32_t)fetch_limit);

        while (result && result->next())
        {
            ChatMessage msg;
            msg.message_id = result->getUint64(0);
            msg.thread_id  = result->getUint64(1);
            msg.sender_id  = result->getUint64(2);
            msg.recv_id    = result->getUint64(3);
            msg.content    = result->getString(4);
            msg.chat_time  = result->getString(5);
            msg.status     = result->getUint32(7);
            page_res->messages.push_back(std::move(msg));
        }
        return page_res;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in LoadChatMsg: {}", e.what());
        return nullptr;
    }
}

bool MysqlDao::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
    auto con = pool_->get();
    if (con == nullptr)
    {
        LOG_ERROR("Failed to get connection from pool");
        return false;
    }

    try
    {
        // 开始事务
        MySQLTransaction::ptr tran = std::static_pointer_cast<MySQLTransaction>(
            con->openTransaction(false));
        if (!tran->begin())
        {
            LOG_ERROR("Failed to begin transaction");
            return false;
        }

        for (auto& msg : chat_datas)
        {
            int ret =
                tran->getMySQL()->execStmt("INSERT INTO chat_message "
                                           "(thread_id, sender_id, recv_id, "
                                           "content, created_at, updated_at, "
                                           "status) "
                                           "VALUES (?, ?, ?, ?, ?, ?, ?)",
                    (uint64_t)msg->thread_id,
                    (uint64_t)msg->sender_id,
                    (uint64_t)msg->recv_id,
                    msg->content,
                    msg->chat_time,
                    msg->chat_time,
                    (uint32_t)msg->status);

            if (ret != 0)
            {
                LOG_ERROR("Failed to AddChatMsg");
                continue;
            }
            msg->message_id = tran->getMySQL()->getLastInsertId();
        }

        if (!tran->commit())
        {
            LOG_ERROR("Failed to commit transaction");
            return false;
        }
        LOG_INFO("AddChatMsg success");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in AddChatMsg: {}", e.what());
        return false;
    }
}