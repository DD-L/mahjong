/*************************************************************************
	> File Name:    src/engine/session_manager.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/26 6:23:23
 ************************************************************************/

#include <engine/session_manager.h>
#include <engine/log.h>

namespace Engine {
namespace Server {

#ifdef ENGINE_DEBUG
void SessionManager::_debug_message(std::ostringstream&& msg) const {
    elogdebug(std::move(msg.str()));
}
#endif // ENGINE_DEBUG


} // namespace Engine::Server
} // namespace Engine
