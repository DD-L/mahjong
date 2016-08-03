/*************************************************************************
	> File Name:    log.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/15 15:02:01
 ************************************************************************/

#include <iostream>
#include <fstream>
#include <functional>

#include <engine/log.h>
namespace Engine {
namespace Log {

// 日志输出格式化函数。
// 因为在编译时使用的是全路径，所以宏 __FILE__ 得到的也是全路径, 所以有必要
// 重新写个日志输出格式化函数，以替换默认的日志输出格式。
static inline sdata_t output_format(const std::shared_ptr<LogVal>& val) {
    std::ostringstream oss;
    oss << log_tools::time2string(val->now)
        << " ["
        //<< std::right << std::setw(5)
        << val->log_type
        << "] " << val->msg << "\t[tid:"
        << val->tid << "] "
        << basename(val->file_name) 
        << ":" << val->line_num
        << ' ' << val->func_name
        << val->extra
        << std::endl;
    return oss.str();
}

} // namespace Log
} // namespace Engine

using namespace Engine;


void Engine::Log::output_thread(const sdata_t& errlog_filename/* = ""*/) {

#ifndef LOGOUTPUT2
//////////////////////////////////////////////////////////////////
// 输出权重大于等于某个级别的日志
//////////////////////////////////////////////////////////////////
    auto& logoutput = LogOutput_t::get_instance();

    // std::cout 输出权重 大于等于 TRACE 级别的日志
    // 日志格式采用自定义格式
    logoutput.bind(std::cout, makelevel(TRACE), 
        std::bind(Log::output_format, std::placeholders::_1));

    std::ofstream logfile;
    if (errlog_filename != "") {
        logfile.open(errlog_filename, std::ofstream::app);
        assert(logfile);

        // 只输出权重大于等于 ERROR  级别的日志
        // 日志输出格式采用自定义格式
        logoutput.bind(logfile, makelevel(ERROR),
                std::bind(Log::output_format, std::placeholders::_1));
    }

    // 日志输出
    std::shared_ptr<LogVal> val = std::make_shared<LogVal>();
        while (true) {
        logoutput(val);
        // TODO
        // do something
        // ...
    } 
    logoutput.unbind(std::cout);
    return; // end function

#else
//////////////////////////////////////////////////////////////////
// 输出指定权重级别的日志
//////////////////////////////////////////////////////////////////
    auto& logoutput = LogOutput2_t::get_instance();
    // 日志输出到std::cout;
    // 只输出 TRACE DEBUG INFO WARN ERROR FATAL 级别的日志;
    // 日志输出格式采用自定义格式
    logoutput.bind(std::cout, 
            {
                makelevel(TRACE), makelevel(DEBUG),
                makelevel(INFO),  makelevel(WARN),
                makelevel(ERROR), makelevel(FATAL)
            },
            std::bind(Log::output_format, std::placeholders::_1));

    std::ofstream logfile;
    if (errlog_filename != "") {
        logfile.open(errlog_filename, std::ofstream::app);
        assert(logfile);

        // 只输出 ERROR FATAL 的日志
        // 日志输出格式采用自定义格式
        logoutput.bind(logfile, { makelevel(ERROR), makelevel(FATAL) },
            std::bind(Log::output_format, std::placeholders::_1));
    }

    // 日志输出
    std::shared_ptr<LogVal> val = std::make_shared<LogVal>();
        while (true) {
        logoutput(val);
        // TODO
        // do something
        // ...
    } 
    logoutput.unbind(std::cout);
    return; // end function
#endif // ifnot defined(LOGOUTPUT2)
}
