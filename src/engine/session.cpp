/*************************************************************************
	> File Name:    src/engine/session.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/25 15:20:38
 ************************************************************************/

#include <engine/session.h>
#include <engine/session_manager.h>

namespace Engine {
namespace Server {

Session::Session(void)
    : m_connection(nullptr), 
      m_timer(Event::get_instance().get_io_service()) {}

void Session::dropped(void) {
    // 释放自己拥有的 connection
    m_connection.reset();
    
    // 并启动定时器
    auto self(shared_from_this());
    timer_async_wait(
        [this, self] () {
            // 从 SessionManager erase 自己
            // 这意味着将调用 Session 的析构函数
            SessionManager::get_instance().erase(self);
        });
}

void Session::update(Session::shared_connection_t conn) {
    timer_cancel();
    m_connection = conn;
}
void Session::update(Session::shared_uuid_t uuid) {
    timer_cancel();
    m_uuid = uuid;
}
void Session::update(
        Session::shared_connection_t conn, 
        Session::shared_uuid_t uuid) {
    update(uuid, conn);
}
void Session::update(
        Session::shared_uuid_t uuid, 
        Session::shared_connection_t conn) {
    timer_cancel();
    m_connection = conn;
    m_uuid = uuid;
}

inline Session::shared_uuid_t Session::uuid(void) {
    return m_uuid;
}

std::weak_ptr<uuid_t> Session::get_uuid_weak_ptr(void) {
    return std::weak_ptr<uuid_t>(m_uuid);
}

std::weak_ptr<Connection> Session::get_connection_weak_ptr(void) {
    return std::weak_ptr<Connection>(m_connection);
}

void Session::timer_cancel() {
    m_timer_wait_flag.clear();
    boost::system::error_code ec;
    m_timer.cancel(ec);
    // TODO
    if (ec) { (void)ec; }
}

} // namespace Engine::Server
} // namespace::Engine
