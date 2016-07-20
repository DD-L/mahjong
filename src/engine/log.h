/*************************************************************************
	> File Name:    log.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/15 14:48:42
 ************************************************************************/

#ifndef E_LOG_H_1 
#define E_LOG_H_1
#include <engine/typedefine.h>
#include <log/init_simple.h>

namespace Engine {
namespace Log {
// function Engine::output_thread
void output_thread(const sdata_t& errlog_filename = "");
} // namespace Engine::Log
} // namespace Engine


// 特别地, 重新包装下 debug 日志输出
#ifdef ENGINE_DEBUG
    #define elogdebug(log) logdebug(log)
    #define elogdebugEx(log, shared_ptr_extra) logdebugEx(log, shared_ptr_extra)
#else
    #define elogdebug(log) 
    #define elogdebugEx(log) 
#endif

#endif // E_LOG_H_1
