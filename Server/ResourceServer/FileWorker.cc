#include "FileWorker.h"
#include "CSession.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include "base64.h"
#include "ConfigMgr.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"

FileWorker::FileWorker() : _b_stop(false)
{
    _work_thread = std::thread([this]() {
        while (!_b_stop)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _cv.wait(lock, [this]() {
                if (_b_stop)
                {
                    return true;
                }

                if (_task_que.empty())
                {
                    return false;
                }

                return true;
            });

            if (_b_stop)
            {
                break;
            }

            auto task = _task_que.front();
            _task_que.pop();
            task_callback(task);
        }
    });
}

FileWorker::~FileWorker()
{
    _b_stop = true;
    _cv.notify_one();
    _work_thread.join();
}

void FileWorker::PostTask(std::shared_ptr<FileTask> task)
{
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _task_que.push(task);
    }

    _cv.notify_one();
}

void FileWorker::task_callback(std::shared_ptr<FileTask> task)
{
    // ����
    std::string decoded = base64_decode(task->_file_data);

    auto file_path_str = task->_path;
    auto last          = task->_last;
    // std::cout << "file_path_str is " << file_path_str << std::endl;

    boost::filesystem::path file_path(file_path_str);
    boost::filesystem::path dir_path = file_path.parent_path();
    // ��ȡ�����ļ�����������չ����
    std::string filename = file_path.filename().string();

    // Check if directory exists, if not, create it
    if (!boost::filesystem::exists(dir_path))
    {
        if (!boost::filesystem::create_directories(dir_path))
        {
            std::cerr << "Failed to create directory: " << dir_path.string()
                      << std::endl;
            return;
        }
    }

    std::ofstream outfile;
    // ��һ����
    if (task->_seq == 1)
    {
        // ���ļ��������������գ��������򴴽�
        outfile.open(file_path_str, std::ios::binary | std::ios::trunc);
    }
    else
    {
        // ����Ϊ�ļ�
        outfile.open(file_path_str, std::ios::binary | std::ios::app);
    }

    if (!outfile)
    {
        std::cerr << "�޷����ļ�����д�롣" << std::endl;
        return;
    }

    outfile.write(decoded.data(), decoded.size());
    if (!outfile)
    {
        std::cerr << "д���ļ�ʧ�ܡ�" << std::endl;
        return;
    }

    outfile.close();
    if (last)
    {
        std::cout << "�ļ��ѳɹ�����Ϊ: " << task->_name << std::endl;
        // ����ͷ��
        MysqlMgr::GetInstance()->UpdateUserIcon(task->_uid, filename);
        // ��ȡ�û���Ϣ
        auto user_info = MysqlMgr::GetInstance()->GetUser(task->_uid);
        if (user_info == nullptr)
        {
            return;
        }

        // �����ݿ�����д��redis����
        Json::Value redis_root;
        redis_root["uid"]    = task->_uid;
        redis_root["pwd"]    = user_info->pwd_;
        redis_root["name"]   = user_info->name_;
        redis_root["email"]  = user_info->email_;
        redis_root["nick"]   = user_info->nick_;
        redis_root["desc"]   = user_info->desc_;
        redis_root["sex"]    = user_info->sex_;
        redis_root["icon"]   = user_info->icon_;
        std::string base_key = USER_BASE_INFO + std::to_string(task->_uid);
        RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
    }
}
