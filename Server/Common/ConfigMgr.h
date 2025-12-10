#pragma once
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <map>

struct SectionInfo
{
    SectionInfo() {}
    ~SectionInfo() { _section_datas.clear(); }

    SectionInfo(const SectionInfo& src) { _section_datas = src._section_datas; }

    SectionInfo& operator=(const SectionInfo& src)
    {
        if (&src == this)
        {
            return *this;
        }

        this->_section_datas = src._section_datas;
        return *this;
    }

    std::string operator[](const std::string& key)
    {
        if (_section_datas.find(key) == _section_datas.end())
        {
            return "";
        }
        // 这里可以添加一些边界检查
        return _section_datas[key];
    }

    std::string GetValue(const std::string& key)
    {
        if (_section_datas.find(key) == _section_datas.end())
        {
            return "";
        }
        // 这里可以添加一些边界检查
        return _section_datas[key];
    }

    std::map<std::string, std::string> _section_datas;
};

class ConfigMgr
{
  public:
    ~ConfigMgr() { config_map_.clear(); }
    SectionInfo operator[](const std::string& section)
    {
        if (config_map_.find(section) == config_map_.end())
        {
            return SectionInfo();
        }
        return config_map_[section];
    }

    ConfigMgr& operator=(const ConfigMgr& src)
    {
        if (&src == this)
        {
            return *this;
        }

        this->config_map_ = src.config_map_;
        return *this;
    };

    ConfigMgr(const ConfigMgr& src) { this->config_map_ = src.config_map_; }

    static ConfigMgr& Inst()
    {
        static ConfigMgr cfg_mgr;
        return cfg_mgr;
    }

    std::string GetValue(const std::string& section, const std::string& key);
    boost::filesystem::path GetFileOutPath();
    void                    InitPath();

  private:
    ConfigMgr();
    // 存储section和key-value对的map
    std::map<std::string, SectionInfo> config_map_;
    // static目录
    boost::filesystem::path static_path_;
    // bin输出目录
    boost::filesystem::path bin_path_;
};
