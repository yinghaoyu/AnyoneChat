
#include "utils.h"

std::string getCurrentTimestamp() {
    namespace pt = boost::posix_time;
    // 取当前时间为准确时间
    pt::ptime now = pt::second_clock::local_time();
    // 时间格式化
    std::ostringstream oss;
    static std::locale loc(std::locale::classic(),
        new pt::time_facet("%Y-%m-%d %H:%M:%S"));
    oss.imbue(loc);
    oss << now;
    return oss.str();
}