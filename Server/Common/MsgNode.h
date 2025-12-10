#pragma once

#include <boost/asio.hpp>
#include <iostream>

using namespace std;
using boost::asio::ip::tcp;

class LogicSystem;
class LogicWorker;
class MsgNode
{
  public:
    MsgNode(short max_len) : total_len_(max_len), cur_len_(0)
    {
        data_             = new char[total_len_ + 1]();
        data_[total_len_] = '\0';
    }

    ~MsgNode()
    {
        std::cout << "MsgNode dtor~" << endl;
        delete[] data_;
    }

    void Clear()
    {
        ::memset(data_, 0, total_len_);
        cur_len_ = 0;
    }

    short cur_len_;
    short total_len_;
    char* data_;
};

class RecvNode : public MsgNode
{
    friend class LogicSystem;
    friend class LogicWorker;

  public:
    RecvNode(short max_len, short msg_id);

  private:
    short msg_id_;
};

class SendNode : public MsgNode
{
    friend class LogicSystem;
    friend class LogicWorker;

  public:
    SendNode(const char* msg, short max_len, short msg_id);

  private:
    short msg_id_;
};
