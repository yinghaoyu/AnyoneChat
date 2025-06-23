#pragma once
#include <string>
#include <vector>
struct UserInfo
{
    UserInfo()
        : name_(""),
          pwd_(""),
          uid_(0),
          email_(""),
          nick_(""),
          desc_(""),
          sex_(0),
          icon_(""),
          back_("")
    {}
    std::string name_;
    std::string pwd_;
    int         uid_;
    std::string email_;
    std::string nick_;
    std::string desc_;
    int         sex_;
    std::string icon_;
    std::string back_;
};

struct ApplyInfo
{
    ApplyInfo(int uid, std::string name, std::string desc, std::string icon,
        std::string nick, int sex, int status)
        : uid_(uid),
          name_(name),
          desc_(desc),
          icon_(icon),
          nick_(nick),
          sex_(sex),
          status_(status)
    {}

    int         uid_;
    std::string name_;
    std::string desc_;
    std::string icon_;
    std::string nick_;
    int         sex_;
    int         status_;
};

// 聊天线程信息
struct ChatThreadInfo
{
    int         _thread_id;
    std::string _type;      // "private" or "group"
    int         _user1_id;  // 私聊时对应 private_chat.user1_id；群聊时设为 0
    int         _user2_id;  // 私聊时对应 private_chat.user2_id；群聊时设为 0
};

//聊天消息信息
struct ChatMessage {
	int message_id;
	int thread_id;
	int sender_id;
	int recv_id;
	std::string unique_id;
	std::string content;
	std::string chat_time;
	int status;
};

// 查询结果结构，增加next_cursor字段
struct PageResult {
	std::vector<ChatMessage> messages;
	bool load_more;
	int next_cursor;  // 本页最后一条message_id，用于下次查询
};
