#ifndef E_HEARTBEAT_H_1
#define E_HEARTBEAT_H_1
/*************************************************************************
	> File Name:    src/engine/heartbeat.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/5 15:16:15
 ************************************************************************/
#include <engine/typedefine.h>

namespace Engine {

// 线程安全的 单向 tcp heartbeat 

// Heartbeat::Sender     发送方
// Heartbeat::Receiver   接收方

// 接收方 判断 发送方 是否在线

// Engine::Heartbeat
namespace Heartbeat {

typedef boost::posix_time::time_duration duration_type;

// Engine::Heartbeat::Detals
namespace Details {

class heartbeat {
public:
    heartbeat(io_service::strand& strand, uint16_t interval)
        : m_strand(strand), 
          m_timer(strand.get_io_service()), 
          m_default_interval(interval) {}
    heartbeat(const heartbeat&) = delete;
    heartbeat& operator= (const heartbeat&) = delete;
    virtual ~heartbeat(void) {}
    // 重置默认周期, thread-safe 
    void set_default_interval(uint16_t default_interval) {
        m_default_interval = default_interval;
    }
    uint16_t defualt_interval(void) const {
        return m_default_interval;
    }
    
    // 取消异步定时器
    void cancel(void) {
        boost::system::error_code ec;
        m_timer.cancel(ec);
        if (ec) { (void)ec; }
    }
    void cancel(boost::system::error_code& ec) {
        m_timer.cancel(ec);
    }
protected:
    io_service::strand&   m_strand;
    deadline_timer        m_timer;
    std::atomic<uint16_t> m_default_interval; 
};

} // namespace Engine::Heartbeat::Detals

class Sender : public Details::heartbeat { // Sender is nocopyable
public:
    Sender(io_service::strand& strand, uint16_t interval)
        : Details::heartbeat(strand, interval) {}
    

    //////////////////////////////////////
    // Functions inherit from Details::heartbeat
    //////////////////////////////////////
    //
    // virtual ~Sender(void);
    // Sender(const SenVjer&) = delete;
    // Sender& operator= (const Sender&) = delete;
    // // 重置默认周期 thread-safe  
    // void set_default_interval(uint16_t default_interval);
    // // 获取当前默认周期
    // uint16_t defualt_interval(void) const;
    // // 取消异步定时器
    // void cancel(void);
    // void cancel(boost::system::error_code& ec);


    // 启动异步计时器并计数（过期回调）；
    // 过期回调函数签名 handler(void); 
    template<typename ExpiredHandler>
    bool async_wait(ExpiredHandler handler) {
        return
        async_wait(boost::posix_time::seconds(m_default_interval), handler);
    }
    // 启动异步计时器并计数（自定义过期时长，过期回调）；
    template<typename ExpiredHandler>
    bool async_wait(const duration_type& expiry_time, ExpiredHandler handler) {
        boost::system::error_code ec;
        m_timer.expires_from_now(expiry_time, ec);
        if (ec) {
            // TODO
            return false;
        }
        //m_timer.async_wait(handler);
        m_timer.async_wait(m_strand.wrap(
                    std::bind(&Sender::do_expiredhandler<ExpiredHandler>, 
                              this, std::placeholders::_1, handler)));
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

private:
    //////////////////////////////////////
    // Variables inherit from Details::heartbeat
    //////////////////////////////////////
    //
    // io_service::strand&   m_strand;
    // deadline_timer        m_timer;
    // std::atomic<uint16_t> m_default_interval; 

}; // class Engine::Heartbeat::Sender


class Receiver : public Details::heartbeat { // Receiver is nocopyable
public:
    Receiver(io_service::strand& strand, 
             uint16_t default_interval, 
             uint16_t max_lose)
        : Details::heartbeat(strand, default_interval),
          m_counter(0),
          m_max_lose(max_lose) {}

    //////////////////////////////////////
    // Functions inherit from Details::heartbeat
    //////////////////////////////////////
    //
    // virtual ~Receiver(void);
    // Receiver(const Receiver&) = delete;
    // Receiver& operator= (const Receiver&) = delete;
    // // 重置默认周期 thread-safe  
    // void set_default_interval(uint16_t default_interval);
    // // 获取当前默认周期
    // uint16_t defualt_interval(void) const;
    // // 取消异步定时器
    // void cancel(void);
    // void cancel(boost::system::error_code& ec);


    // 启动异步计时器并计数（过期回调）；
    // 过期回调函数签名 
    //      handler(bool is_dropped); // is_dropped == true 说明对方掉线了
    //
    template<typename ExpiredHandler>
    bool async_wait(ExpiredHandler handler) {
        return
        async_wait(
                boost::posix_time::seconds(m_default_interval), 
                handler
        );
    }
    // 启动异步计时器并计数（自定义过期时长，过期回调）；
    template<typename ExpiredHandler>
    bool async_wait(const duration_type& expiry_time, ExpiredHandler handler) {
        boost::system::error_code ec;
        m_timer.expires_from_now(expiry_time, ec);
        if (ec) {
            // TODO
            return false;
        }
        //m_timer.async_wait(handler);
        m_timer.async_wait(m_strand.wrap(
                    std::bind(&Receiver::do_expiredhandler<ExpiredHandler>, 
                              this, std::placeholders::_1, handler)));
        return true;
    }

    // 重置计数器
    void reset(void) {
        m_counter = 0;
    }

    // Sender 是否已经掉线
    bool is_dropped(void) const {
        return m_counter > m_max_lose;
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
        ++m_counter;
        handler(is_dropped());
    }
private:
    //////////////////////////////////////
    // Variables inherit from Details::heartbeat
    //////////////////////////////////////
    //
    // io_service::strand&   m_strand;
    // deadline_timer        m_timer;
    // std::atomic<uint16_t> m_default_interval; 

    std::atomic<uint16_t> m_counter; // 超时 前++, reset =0
    uint16_t              m_max_lose;
}; // class Engine::Heartbeat::Receiver 

} // namespace Engine::Heartbeat
} // namespace Engine

#endif // E_HEARTBEAT_H_1
