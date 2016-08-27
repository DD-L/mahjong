#ifndef E_SESSION_MANAGER_H_1
#define E_SESSION_MANAGER_H_1
/*************************************************************************
	> File Name:    src/engine/session_manager.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 16:57:58
 ************************************************************************/
#include <engine/session.h>
#include <engine/uuid.h>
#include <engine/event.h>
#include <engine/server.h>
#include <engine/log.h>

namespace Engine {
namespace Server {
// 线程安全的单件
// class Engine::Server::SessionManager
class SessionManager {
public:
    typedef std::shared_ptr<Connection> shared_connection_t;
    typedef std::shared_ptr<uuid_t>     shared_uuid_t;
    typedef std::shared_ptr<Session>    shared_session_t;
public:
    static SessionManager& get_instance() {
        static SessionManager sm;
        return sm;
    }
    template <class Func>
    void deliver_uuid(shared_uuid_t uuid, shared_connection_t conn, 
            Func callback) {
        Event::get_instance().strand_post(m_strand,
                &SessionManager::details_deliver_uuid<Func>,
                this, uuid, conn, callback);
    } 
    void erase(shared_session_t session) {
        Event::get_instance().strand_post(m_strand,
                &SessionManager::details_erase, this, session);
    }
private:
    template<class Func>
    void details_deliver_uuid (
            shared_uuid_t uuid, 
            shared_connection_t conn,
            Func callback ) {

        // 判断 uuid 是否在 map 中,
        //  如果在 （uuid 设备已经在线）
        auto&& it = m_session_map.find(*uuid);
        if (it == m_session_map.end()) {
            // uuid 设备之前 不在线
#ifdef ENGINE_DEBUG
            {
                std::ostringstream oss;
                oss << "uuid=" << *uuid << " Come online.";
                _debug_message(std::move(oss));
            }
#endif // ENGINE_DEBUG
            // 新建一个 pair 插入 map 中去
            m_session_map.emplace (
                    *uuid,
                    // 生成一个 session
                    // Engine::Server::Server::session_type
                    // 并关联到 conn 中
                    conn->session(conn->session_generator()) // conn->session
            ); // m_session_map.emplace
            // 更新 session 持有的 uuid 和 conn
            conn->session()->update(uuid, conn);
        }
        else {
            // 断线重连
#ifdef ENGINE_DEBUG
            {
                std::ostringstream oss;
                oss << "uuid=" << *uuid << " Reconnection.";
                _debug_message(std::move(oss));
            }
#endif // ENGINE_DEBUG
            // 只需给新的连接 关联 旧的 session 即可
            conn->session(it->second);
            // session 中的 uuid 无需更新
            // 只需更新 connnection
            conn->session()->update(conn);
        }

        callback(); 
    }
    void details_erase(shared_session_t session) {
        // map 的 erase 会析构对象
        m_session_map.erase(*session->uuid()); 
    }
private:
#ifdef ENGINE_DEBUG
    void _debug_message(std::ostringstream&& msg) const;
#endif // ENGINE_DEBUG
private:
    explicit SessionManager(void)
        : m_strand(Event::get_instance().get_io_service()) {}
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator= (const SessionManager&) = delete;
private:
    io_service::strand                        m_strand;
    std::map<uuid_t, shared_session_t> m_session_map;
}; // class Engine::Server::SessionManager
} // namepsace Server
} // namespace Engine

#endif // E_SESSION_MANAGER_H_1
