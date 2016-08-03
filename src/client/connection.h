#ifndef EC_CONNECTION_H_1
#define EC_CONNECTION_H_1
/*************************************************************************
	> File Name:    src/client/connection.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/21 17:23:55
 ************************************************************************/

#include <engine/typedefine.h>
#include <engine/e_packet.h>

// std::shared_mutex 直到 C++17 才被支持
#include <boost/thread/shared_mutex.hpp>

namespace Engine {
namespace Client {

// 线程安全的单件
class Connection {
private:
    Connection(void);
    Connection(const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;
private:
    static std::mutex _debug_message_callback_mutex;
    static std::function<void(const char*)> _debug_message_callback;
public:
    static void DebugMessage(sdata_t&& message) {
        if (Connection::_debug_message_callback) {
            std::unique_lock<std::mutex> 
                lock(Connection::_debug_message_callback_mutex);
            Connection::_debug_message_callback(message.c_str());
        }
    }
    // 注册 debug message 回调
    template<typename CallBack>
    static void register_debug_message_callback(CallBack callback) {
        std::unique_lock<std::mutex> lock(_debug_message_callback_mutex);
        _debug_message_callback = callback;
    }
public:
    static Connection& Instance(void) {
        static Connection instance;
        return instance;
    }
    // 注册 receive message 后的回调函数
    // 配合 std::bind 它可以适配任意函数签名的回调函数
    template<typename CallBack>
    void register_received_callback(CallBack callback) {
        // 共享锁的写锁
        // std::shared_mutex 直到C++17 才被支持
        std::unique_lock<boost::shared_mutex> lock(m_received_callback_mutex);
        this->m_received_callback = callback;
    }


    // 注册 断线后的回调, 一般都是重连
    template<typename CallBack>
    void register_disconnect_callback(CallBack callback) {
        std::unique_lock<boost::shared_mutex> lock(m_disconnect_callback_mutex);
        this->m_disconnect_callback = callback;
    }

    // 注册 读取失败后的回调，给出失败原因和失败码
    template<typename CallBack>
    void register_read_error_callback(CallBack callback) {
        std::unique_lock<boost::shared_mutex> lock(m_read_error_callback_mutex);
        this->m_read_error_callback = callback;
    }
    /*
    // 注册 写失败后的回调，给出失败原因和失败码
    template<typename CallBack>
    void register_write_error_callback(CallBack callback) {
        std::unique_lock<boost::shared_mutex> lock(m_write_error_callback_mutex);
        this->m_write_error_callback = callback;
    }
    */
    virtual ~Connection();

    // 启动 io_service
    // 非阻塞
    void run(CallbackConstCharPtr callback);
    // 连接游戏服务器
    // 非阻塞
    void start(const sdata_t& server_name, 
            const sdata_t& server_port, 
            CallbackInt callback);

    // callback(int error_code, 
    //          const char* error_code.message, 
    //          const char* data, size_t data_len,
    //          size_t byte_transferred)
    void send_message(data_t&& data, 
            CallbackIntConstCharPtrConstCharPtrSizeSize callback);

    void stop();
private:
    void thread_working(CallbackConstCharPtr callback);
private:
    void cancel_socket() throw();
    // 参数 exec_callback 是否执行断线回调函数
    void close_socket(bool exec_callback = false) throw();
private:
    void resolve_handler(const boost::system::error_code& ec,
            tcp::resolver::iterator i, CallbackInt callback);

    void connect_handler(const boost::system::error_code& ec,
            tcp::resolver::iterator i, CallbackInt callback);

    void write_handler(const boost::system::error_code& error,
        std::size_t bytes_transferred, shared_request_type send_data, 
        CallbackIntConstCharPtrConstCharPtrSizeSize callback);

    void read_handler(const boost::system::error_code& error,
            std::size_t bytes_transferred, shared_reply_type e_reply,
            shared_data_type __data_rest);
private:
    void do_receive_message(void);
    void respond_server_ping(void); 
    void ping_handler(const boost::system::error_code& error,
            std::size_t bytes_transferred);
private:
    inline shared_reply_type make_shared_reply(
            std::size_t length = readbuffer::length_handshake) {
        //return std::make_shared<reply>(max_length);
        return std::make_shared<reply>(length);
    }
    inline shared_request_type make_shared_request(const request& e_request) {
        return std::make_shared<request>(std::move(e_request));
    }
private:
    const int unpack_data(data_t& plain,
            const std::size_t e_pack_length,
            const reply& reply, bool is_zip = false);
    inline void call_received_callback(const char* data, std::size_t len) {
        m_received_callback_mutex.lock_shared();
        m_received_callback(data, len);
        m_received_callback_mutex.unlock_shared();
    }
private:
    io_service    m_ios;
    tcp::resolver m_resolver;
    tcp::socket   m_socket;
    std::thread   m_io_thread;
    std::mutex    m_run_mutex;

    // 读写锁, 读的频率远大于写的频率
    boost::shared_mutex m_received_callback_mutex;
    // std::shared_mutex 直到 c++17 才被支持
    std::function<void(const char*, std::size_t)> m_received_callback;

    boost::shared_mutex m_disconnect_callback_mutex;
    std::function<void(void)> m_disconnect_callback;

    boost::shared_mutex m_read_error_callback_mutex;
    std::function<void(const char*, int)> m_read_error_callback;

    //boost::shared_mutex m_write_error_callback_mutex;
    //std::function<void(const char*, int)> m_write_error_callback;


}; // class Connection

} // namespace Engine::Client
} // namespace Engine

#endif //EC_CONNECTION_H_1

