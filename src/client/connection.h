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
#include <engine/heartbeat.h> // for Engine::Heartbeat::Sender
#include <engine/uuid.h>

// std::shared_mutex 直到 C++17 才被支持
#include <boost/thread/shared_mutex.hpp>

namespace Engine {
namespace Client {

// 线程安全的单件
class Connection {
public:
    static Engine::test_and_set_value<Engine::uuid_t> m_uuid;
private:
    explicit Connection(io_service& io_service);
    Connection(const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;
private:
    static std::mutex _debug_message_callback_mutex;
    static std::function<void(const char*)> _debug_message_callback;
public:
    // 初始化多线程, 最多只能设置一次，否则会断言失败
    static void inital_multithread_setting(const size_t thread_n) {
        assert(m_write_in_multithreads.set(thread_n > 1 ? true : false));
    }
    static void DebugMessage(sdata_t&& message) {
        if (Connection::_debug_message_callback) {
            std::lock_guard<std::mutex> 
                lock(Connection::_debug_message_callback_mutex);
            Connection::_debug_message_callback(message.c_str());
        }
    }
    // 注册 debug message 回调
    template<typename CallBack>
    static void register_debug_message_callback(CallBack callback) {
        std::lock_guard<std::mutex> lock(_debug_message_callback_mutex);
        _debug_message_callback = callback;
    }
public:
    static Connection& Instance(void) {
        static io_service io_service;
        static Connection instance(io_service);
        return instance;
    }
    // 注册 receive message 后的回调函数
    // 配合 std::bind 它可以适配任意函数签名的回调函数
    template<typename CallBack>
    void register_received_callback(CallBack callback) {
        // 共享锁的写锁
        // std::shared_mutex 直到C++17 才被支持
        std::lock_guard<boost::shared_mutex> lock(m_received_callback_mutex);
        this->m_received_callback = callback;
    }


    // 注册 断线后的回调, 一般都是重连
    template<typename CallBack>
    void register_disconnect_callback(CallBack callback) {
        std::lock_guard<boost::shared_mutex> lock(m_disconnect_callback_mutex);
        this->m_disconnect_callback = callback;
    }

    // 注册 读取失败后的回调，给出失败原因和失败码. 断线回调先于此回调
    template<typename CallBack>
    void register_read_error_callback(CallBack callback) {
        std::lock_guard<boost::shared_mutex> lock(m_read_error_callback_mutex);
        this->m_read_error_callback = callback;
    }
    // 注册 写失败后的回调，给出失败原因和失败码
    template<typename CallBack>
    void register_write_error_callback(CallBack callback) {
        std::lock_guard<boost::shared_mutex> lock(m_write_error_callback_mutex);
        this->m_write_error_callback = callback;
    }

    virtual ~Connection();
    // 重置 heartbeat ping 周期, (秒)
    void set_ping_interval(uint16_t interval);

    // 启动 io_service
    // 非阻塞
    void run(CallbackConstCharPtr callback);
    // 连接游戏服务器
    // 非阻塞
    void start(const sdata_t& server_name, 
            const sdata_t& server_port, 
            CallbackInt callback);

    // callback(size_t byte_transferred)
    void send_message(data_t&& data, CallbackSize callback);

    void stop();
private:
    void thread_working(CallbackConstCharPtr callback);
    inline io_service& get_io_service(void) {
        return m_tcp_socket.get_io_service();
    }
    // 获取 uuid 指针
    const_byte_ptr uuid_data(void);
    // 将 ping interval 转成网络字节序
    data_t ping_interval_binary(void);
private:
    void cancel_socket() throw();
    // 参数 exec_callback 是否执行断线回调函数
    void close_socket(bool exec_callback = false) throw();
private:
    void resolve_handler(const boost::system::error_code& ec,
            tcp::resolver::iterator i, CallbackInt callback);

    void connect_handler(const boost::system::error_code& ec,
            tcp::resolver::iterator i, CallbackInt callback);

    // 有用户每次 send_message 传进来，所以这里就不需要了
    //void write_handler(std::size_t bytes_transferred) {
    //    (void) bytes_transferred;
    //} 

    void read_handler(
            std::size_t       bytes_transferred, 
            shared_reply_type e_reply,
            shared_data_type  __data_rest);
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

    void do_heartbeat_loop(void);

    // on_read_error
    void tcp_socket_read_some_error_handler(
            const boost::system::error_code& error);
    // on_write_error
    // 重载后无法解决std::bind 问题所以还是命名不同名字
    // tcp_socket_write_error_handler_1
    template <typename ConstBufferSequence,
              typename RewriteHandler>
    void tcp_socket_write_error_handler_1(
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const ConstBufferSequence&       __send_data,
            RewriteHandler                   rewrite_succeeded_handler) {

        auto&& send_data = asio::buffer(__send_data);

        std::size_t send_data_size = asio::buffer_size(send_data);
        const_byte_ptr rest_data_ptr 
            = asio::buffer_cast<const_byte_ptr>(send_data) + bytes_transferred;

        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                send_data_size,
                [&] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write(
                        asio::buffer (
                            rest_data_ptr,
                            send_data_size - bytes_transferred),
                        rewrite_succeeded_handler
                        //std::bind(&Connection::write_handler,
                        //    this, std::placeholders::_1)
                    ); // tcp_socket_async_write
                },
                // 未写完的数据的起始地址
                rest_data_ptr                        
        ); // __tcp_socket_write_error_wrapper
    }
    // tcp_socket_write_error_handler_2
    template <typename RewriteHandler>
    void tcp_socket_write_error_handler_2 (
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const shared_data_type&          send_data,
            RewriteHandler                   rewrite_succeeded_handler) {
            
        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                send_data->size(),
                [&] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write (
                        make_shared_data({
                                send_data->begin() + bytes_transferred,
                                send_data->end()
                        }),
                        //std::bind(&Connection::write_handler,
                        //    this, std::placeholders::_1)
                        rewrite_succeeded_handler
                    ); // tcp_socket_async_write
                },
                // 未写完的数据的起始地址
                &(*send_data)[0] + bytes_transferred
        ); // __tcp_socket_write_error_wrapper
    }
    // tcp_socket_write_error_handler_3
    template <typename RewriteHandler>
    void tcp_socket_write_error_handler_3 (
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const shared_request_type&       send_data,
            RewriteHandler                   rewrite_succeeded_handler) {
            
        vdata_t&& origin_data = get_vdata_from_packet(*send_data);
        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                origin_data.size(),
                [&] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write (
                        make_shared_data(
                                origin_data.begin() + bytes_transferred,
                                origin_data.end()
                        ),
                        rewrite_succeeded_handler
                        //std::bind(&Connection::write_handler,
                        //    this, std::placeholders::_1));
                    ); // tcp_socket_async_write
                },
                // 未写完的数据的起始地址
                &origin_data[0] + bytes_transferred
        ); // __tcp_socket_write_error_wrapper
    }

private:
    template <typename WriteRestFunc>
    void __tcp_socket_write_error_wrapper(
            const boost::system::error_code& error, 
            size_t                           bytes_transferred,
            size_t                           data_size,
            WriteRestFunc                    wrf,
            // 未写完的数据的起始地址
            const_byte_ptr                   rest_data_ptr) { 
        
        // 连接还可用，但因为某些原因没写入完整
        if ((bytes_transferred > 0)                          &&
            (bytes_transferred < data_size)                   &&
            (error != boost::asio::error::eof)               && 
            (error != boost::asio::error::connection_reset)  &&
            (error != boost::asio::error::operation_aborted) &&
            (m_tcp_socket.is_open())) {
            
            std::ostringstream msg;
            msg << "Data is not completely written ("
                << bytes_transferred << " bytes have been written last time). "
                "but the connection is still available. Continue to write...";
            DebugMessage(std::move(msg.str()));
            // 继续写入剩下的数据
            wrf();
        }
        else {
            std::ostringstream msg;
            msg << "write message error. " << error.message() 
                    << ", value=" << error.value() 
                    << ", (" << bytes_transferred 
                    << " bytes have been written last time). "
                    << "cancel this, this=" << this;
            DebugMessage(std::move(msg.str()));

            // close
            close_socket(true);

            // 把没有写完的数据
            //{
            //const_byte_ptr rest_data_ptr,
            //size_t         data_size - bytes_transferred
            //}
            
            // 反馈给客户
            if (m_write_error_callback) {
                m_write_error_callback(error.message().c_str(), error.value(), 
                        rest_data_ptr, data_size - bytes_transferred); 
            }
        }
    }

protected:
    // tcp 异步读取方法
    // 必须保证直到读取结束，buffer 都存在于内存当中
    // 重载一, 默认参数 ReadErrorHandler
    template <typename MutableBufferSequence, typename ReadSucceededHandler>
    void tcp_socket_async_read_some(
            const MutableBufferSequence& buffer,
            ReadSucceededHandler         succeeded_handler) {
        return tcp_socket_async_read_some(
                buffer,
                succeeded_handler,
                std::bind(
                    &Connection::tcp_socket_read_some_error_handler,
                    this,
                    std::placeholders::_1)
        );
    }
    // 重载二
    template <
        typename MutableBufferSequence, 
        typename ReadSucceededHandler,
        typename ReadErrorHandler
    >
    void tcp_socket_async_read_some(
            const MutableBufferSequence& buffer,
            ReadSucceededHandler         succeeded_handler,
            ReadErrorHandler             error_handler) {

        this->m_tcp_socket.async_read_some (
                buffer,
                m_strand.wrap (
                    boost::bind (
                        &Connection::tcp_socket_async_read_some_handler
                        <ReadSucceededHandler, ReadErrorHandler>,
                        this, _1, _2, 
                        succeeded_handler, 
                        error_handler
                    ) // boost::bind
                ) // m_strand.wrap
        ); // this->m_tcp_socket.async_read_some

        // 执行 异步心跳循环
        //do_heartbeat_loop();
    } // end - function tcp_socket_async_read_some
private:
    // 异步读回调处理
    template<
        typename ReadOrWriteSucceededHandler,
        typename ReadOrWriteErrorHandler>
    void tcp_socket_async_read_some_handler(
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            ReadOrWriteSucceededHandler      succeeded_handler,
            ReadOrWriteErrorHandler          error_handler) 
    {
        //this->m_heartbeat.cancel();
        // 执行 异步心跳循环
        do_heartbeat_loop();
        if (error) {
            // read error
            error_handler(error);
            return;
        }

        // 读取成功
        succeeded_handler(bytes_transferred);
    }

protected:
    // tcp 异步写方法

    // 重载一
    // 第一个参数是 asio::buffer, 
    // 调用时必须保证 buffer 的数据直到回调仍然在内存中
    //
    // 重载 1.1
    template<typename ConstBufferSequence>
    void tcp_socket_async_write(const ConstBufferSequence& buffer) {
        return tcp_socket_async_write(
            buffer,
            [] (std::size_t) {}
        );
    }
    // 重载 1.2 默认参数 WriteErrorHandler
    template<typename ConstBufferSequence, typename WriteSucceededHandler>
    void tcp_socket_async_write(
            const ConstBufferSequence& buffer,
            WriteSucceededHandler      succeeded_handler) {
        return tcp_socket_async_write(
            buffer,
            succeeded_handler,
            std::bind(&Connection::tcp_socket_write_error_handler_1
                      <ConstBufferSequence, WriteSucceededHandler>,
                      this,
                      std::placeholders::_1, // error_code
                      std::placeholders::_2, // bytes_transferred
                      std::placeholders::_3, // buffer
                      std::placeholders::_4) // rewrite_succeeded_handler
        );
    }
    // 重载 1.3
    template<
        typename ConstBufferSequence,
        typename WriteSucceededHandler,
        typename WriteErrorHandler
    >
    void tcp_socket_async_write(
            const ConstBufferSequence& buffer, 
            WriteSucceededHandler      succeeded_handler,
            WriteErrorHandler          error_handler) {

        __write_wrapper ( 
                [&] (void) {
                    boost::asio::async_write (
                            this->m_tcp_socket, 
                            buffer,
                            m_strand.wrap (
                                boost::bind (
                                    &Connection::tcp_socket_async_write_handler
                                    //<ConstBufferSequence,
                                    // ReadSucceededHandler, ReadErrorHandler>,
                                    <decltype(std::cref(buffer)),
                                     decltype(succeeded_handler),
                                     decltype(error_handler)>,
                                    this, _1, _2,
                                    // buffer 直到回调一直存在, 所以可以用引用
                                    std::cref(buffer),
                                    succeeded_handler,
                                    error_handler
                                ) // boost::bind
                            ) // m_strand.wrap
                    ); // boost::asio::async_write
        }); // __write_wrapper 
    }
    
    // 重载二
    // 第一个参数是 shared_data_type 
    // 重载2.1
    void tcp_socket_async_write(const shared_data_type& shared_data) {
        return tcp_socket_async_write(shared_data, [] (std::size_t) {});
    }
    // 重载2.2
    template <typename WriteSucceededHandler>
    void tcp_socket_async_write(
            const shared_data_type&  shared_data,
            WriteSucceededHandler    succeeded_handler) {
        return tcp_socket_async_write(shared_data, succeeded_handler,
                std::bind(&Connection::tcp_socket_write_error_handler_2
                        <WriteSucceededHandler>,
                        this,
                        std::placeholders::_1, // error_code
                        std::placeholders::_2, // bytes_transferred
                        std::placeholders::_3, // shared_data
                        std::placeholders::_4) // rewrite_succeeded_handler
                );
    }
    // 重载2.3
    template <typename WriteSucceededHandler, typename WriteErrorHandler>
    void tcp_socket_async_write(
            const shared_data_type& shared_data,
            WriteSucceededHandler   succeeded_handler,
            WriteErrorHandler       error_handler) {

        __write_wrapper ( 
                [&] (void) {
                    boost::asio::async_write (
                            this->m_tcp_socket, 
                            asio::buffer(*shared_data),
                            m_strand.wrap (
                                boost::bind (
                                    &Connection::tcp_socket_async_write_handler
                                    <shared_data_type,
                                    //ReadSucceededHandler, ReadErrorHandler>,
                                    decltype(succeeded_handler),
                                    decltype(error_handler)>,
                                    this, _1, _2,
                                    shared_data,
                                    succeeded_handler,
                                    error_handler
                                ) // boost::bind
                            ) // m_strand.wrap
                    ); // boost::asio::async_write
        }); // __write_wrapper
    }
    // 重载三
    // 第一个参数是 shared_request_type 
    // 重载 3.1
    void tcp_socket_async_write(const shared_request_type& shared_data) {
        return tcp_socket_async_write(shared_data, [] (std::size_t) {});        
    }
    // 重载 3.2
    template <typename WriteSucceededHandler>
    void tcp_socket_async_write(
            const shared_request_type& shared_data,
            WriteSucceededHandler      succeeded_handler) {
        return tcp_socket_async_write(shared_data, succeeded_handler,
                std::bind(&Connection::tcp_socket_write_error_handler_3
                        <WriteSucceededHandler>,
                        this,
                        std::placeholders::_1, // error_code
                        std::placeholders::_2, // bytes_transferred
                        std::placeholders::_3, // shared_data
                        std::placeholders::_4) // rewrite_succeeded_handler
                );
    }
    // 重载 3.3
    template <typename WriteSucceededHandler, typename WriteErrorHandler>
    void tcp_socket_async_write(
            const shared_request_type&   shared_data,
            WriteSucceededHandler        succeeded_handler,
            WriteErrorHandler            error_handler) {

        __write_wrapper ( 
                [&] (void) {
                    boost::asio::async_write (
                            this->m_tcp_socket, 
                            shared_data->buffers(),
                            m_strand.wrap (
                                boost::bind (
                                    &Connection::tcp_socket_async_write_handler
                                    <shared_request_type,
                                    //ReadSucceededHandler, ReadErrorHandler>,
                                    decltype(succeeded_handler),
                                    decltype(error_handler)>,
                                    this, _1, _2,
                                    shared_data,
                                    succeeded_handler,
                                    error_handler
                                ) // boost::bind
                            ) // m_strand.wrap
                    ); // boost::asio::async_write
        }); // __write_wrapper
    }

private:
    template <typename WriteFunc>
    void __write_wrapper(WriteFunc wf) {
        if (m_write_in_multithreads) {
            m_tcp_socket_write_mutex.lock();
        }
        // asio::async_write();
        wf();
        // 执行 异步心跳循环
        //do_heartbeat_loop();
    }

    // 异步写回调处理
    template<
        typename ConstBufferSequenceOrSharedData,
        typename ReadOrWriteSucceededHandler,
        typename ReadOrWriteErrorHandler>
    void tcp_socket_async_write_handler(
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            ConstBufferSequenceOrSharedData  buffer_or_shared_data,
            ReadOrWriteSucceededHandler      succeeded_handler,
            ReadOrWriteErrorHandler          error_handler) 
    {
        if (m_write_in_multithreads) {
            m_tcp_socket_write_mutex.unlock();
        }

        //this->m_heartbeat.cancel();
        // 执行 异步心跳循环
        do_heartbeat_loop();
        if (error) {
            // write error
            error_handler(
                    error, 
                    bytes_transferred, 
                    buffer_or_shared_data, // 
                    succeeded_handler      //  rewrite_succeeded_handler
            );
            return;
        }

        // 写入成功
        succeeded_handler(bytes_transferred);
    }
private:
    io_service::strand m_strand;
    Heartbeat::Sender  m_heartbeat;
    tcp::resolver      m_resolver;
    tcp::socket        m_tcp_socket;
    std::thread        m_io_thread;
    std::mutex         m_run_mutex;

    // 读写锁, 读的频率远大于写的频率
    boost::shared_mutex m_received_callback_mutex;
    // std::shared_mutex 直到 c++17 才被支持
    std::function<void(const char*, std::size_t)> m_received_callback;

    boost::shared_mutex m_disconnect_callback_mutex;
    std::function<void(void)> m_disconnect_callback;

    boost::shared_mutex m_read_error_callback_mutex;
    std::function<void(const char*, int)> m_read_error_callback;

    boost::shared_mutex m_write_error_callback_mutex;
    std::function<void(const char*, int, const byte*, size_t)> 
        m_write_error_callback;
private:
    static Engine::test_and_set_value<bool>  m_write_in_multithreads;
    // 不需要写锁
    UseStdMutex<false>   m_tcp_socket_write_mutex; 
    // 只有当 m_tcp_socket_write_mutex 的类型为 UseStdMutex<true>
    // 且 m_write_in_multithreads 的值为 true 时, 才会开启锁的功能
}; // class Engine::Client::Connection

} // namespace Engine::Client
} // namespace Engine

#endif //EC_CONNECTION_H_1

