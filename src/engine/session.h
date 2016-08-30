#ifndef E_SESSION_H_1
#define E_SESSION_H_1
/*************************************************************************
	> File Name:    src/engine/session.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 8:14:29
 ************************************************************************/
#include <engine/event.h>
#include <engine/connection.h>
#include <engine/uuid.h>
#include <engine/config_server.h>

namespace Engine {
namespace Server {

// 负责整个业务流程
// 一个客户端设备 对应一个 session
class Session : public std::enable_shared_from_this<Session> {
protected:
    typedef Session                  BASETYPE;
public:
    typedef std::shared_ptr<Connection> shared_connection_t;
    typedef std::shared_ptr<uuid_t>     shared_uuid_t;
    //typedef boost::posix_time::time_duration duration_type;
public:
    virtual ~Session() = default;
    Session(void);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
public:
    // 处理 从客户端发来的数据
    // 需要子类重写
    virtual void data_process(shared_data_type) {}

    // 设备掉线处理
    void dropped(void);

    // 更新持有的 connection
    void update(shared_connection_t conn);
    // 更新持有的 uuid
    void update(shared_uuid_t uuid);
    // 同时更新持有的 uuid 和 connection
    void update(shared_connection_t conn, shared_uuid_t uuid);
    void update(shared_uuid_t uuid, shared_connection_t conn);

    // 获取持有的 uuid 共享指针
    shared_uuid_t uuid(void);
    // 获取持有的 uuid 弱共享指针
    std::weak_ptr<uuid_t> get_uuid_weak_ptr(void);
    // 获取持有的 connection 弱共享指针, 传递时不会改变 connection 的引用计数
    std::weak_ptr<Connection> get_connection_weak_ptr(void);
private:
    // timer.cancel
    void timer_cancel();

    // timer.async_wait
    template<typename ExpiredHandler>
    bool timer_async_wait(ExpiredHandler handler) {

        // 只允许一个异步等待
        if (m_timer_wait_flag.test_and_set()) {
            // 之前已经被 set 过了, 并且没有 clear
            return false;
        }

        auto&& reconnection_delay 
            = Config::get_instance().get_reconnection_delay();
        auto&& expiry_time = boost::posix_time::seconds(reconnection_delay);

        boost::system::error_code ec;
        m_timer.expires_from_now(expiry_time, ec);
        if (ec) {
            // TODO
            return false;
        }
        m_timer.async_wait (
               std::bind (
                   &Session::do_expiredhandler<ExpiredHandler>,
                   shared_from_this(), std::placeholders::_1, handler
               ) // std::bind 
        ); //  m_timer.async_wait
        return true;
    }

private:
    template<typename ExpiredHandler>
    void do_expiredhandler(const boost::system::error_code& error,
                        ExpiredHandler handler) {
        // On error, such as cancellation, return early.
        if (error) return;

        // Timer has expired, but the async- operation's completion handler
        // may have already ran, setting expiration to be in the future.
        if (m_timer.expires_at() > deadline_timer::traits_type::now()) {
            return;
        }

        // The async- operation's completion handler has not ran.
        // expired
        handler();
    }
protected:
    shared_uuid_t        m_uuid = nullptr;
private:
    shared_connection_t  m_connection;
    deadline_timer       m_timer;    
    std::atomic_flag     m_timer_wait_flag = ATOMIC_FLAG_INIT;
}; // class Engine::Server::Session

} // namespace Server
} // namespace Engine

#endif // E_SESSION_H_1
