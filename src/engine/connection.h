#ifndef E_CONNECTION_H_1
#define E_CONNECTION_H_1
/*************************************************************************
	> File Name:    connection.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/17 9:52:17
 ************************************************************************/

#include <engine/typedefine.h>
#include <engine/e_packet.h>
#include <engine/heartbeat.h> // for Engine::Heartbeat::Receiver

namespace Engine {
namespace Server {

// forward decl
class Session; // class Engine::Server::Session;


// 负责网络通信协议部分
class Connection : public std::enable_shared_from_this<Connection> { 
public:
    typedef std::shared_ptr<Connection> pointer;
    typedef Connection                  BASETYPE;
    typedef std::shared_ptr<Session>    shared_session_t;
public:
    // 初始化多线程, 最多只能设置一次，否则会断言失败
    static void inital_multithread_setting(const size_t thread_n) {
        assert(m_write_in_multithreads.set(thread_n > 1 ? true : false));
    }
public:
    explicit Connection(boost::asio::io_service& io_service); 
    virtual ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;
    void start(void) throw (Excpt::ConnectionException);
    /*virtual*/ void close(void) noexcept;
    /*virtual*/ void cancel(void) noexcept; 
    // 需要子类重写
    virtual shared_session_t session_generator(void) {
        return std::make_shared<Session>();
    }
    // 掉线后通知 session
    void dropped_session(void);

    // set session
    inline shared_session_t session(shared_session_t&& session) {
        return this->session(session);
    }
    inline shared_session_t session(shared_session_t& session) {
        return (m_session = session);
    }
    // get session
    inline shared_session_t session(void) {
        return m_session;
    }

    inline tcp::socket& tcp_socket() {
        return m_tcp_socket; 
    }

    // 向客户端发送数据
    // data 的类型须是 Engine::data_t 类型, 推荐传递右值
    template <typename DataType>
    void send_message(DataType&& data) {
        static_assert(
            std::is_same<
                typename std::decay<decltype (data)>::type,
                Engine::data_t 
            >::value,
            "The decay type of 'decltype(data)' should be 'Engine::data_t' !" 
        );
        auto&& shared_reply = make_shared_reply(pack_data(
                    std::forward<DataType>(data)));
        tcp_socket_async_write(shared_reply);
    }
    template <typename DataType, typename SendSucceededHandler>
    void send_message(DataType&& data, 
            SendSucceededHandler&& succeeded_handler) {
        static_assert(
            std::is_same<
                typename std::decay<decltype (data)>::type,
                Engine::data_t 
            >::value,
            "The decay type of 'decltype(data)' should be 'Engine::data_t' !" 
        );
        auto&& shared_reply = make_shared_reply(pack_data(
                    std::forward<DataType>(data)));
        tcp_socket_async_write(shared_reply, 
                std::forward<SendSucceededHandler>(succeeded_handler));
    }
private: // tools
    // 当前 Connection 状态断言
    void assert_status(ConnetionStatus::status_t status) {
        if (this->m_status != status) {
            throw Excpt::wrong_conn_status(status, this->m_status);
        }
    }

    // get heartbeat interval 
    uint16_t heartbeat_interval(uint16_t ping_interval) const;

    // 解包
    const int unpack_data(data_t& plain, const std::size_t e_reply_length,
            const request& request, bool is_zip = false);

private:
    // read succeeded handler
    void read_handler(
            std::size_t         bytes_transferred, 
            shared_request_type e_request,
            shared_data_type    __data_rest);
    // write succeeded handler
    void write_handler(std::size_t  bytes_transferred);
private:
    void do_receive_message(void);
private:

    // client 掉线处理
    void client_dropped_handler(void);

    // 异步心跳循环，
    // 正常情况下只有在 客户端掉线 和 主动 heartbeat.cancel() 时 才会结束循环
    void do_heartbeat_loop(pointer const& self);
    //void do_heartbeat_loop(decltype(shared_from_this())& self);

    // TCP 保证数据可靠到达.
    // 在 read_some 中
    // 如出现任何异常 error != 0 , 则必有 bytes_transferred == 0
    // on_read_some_error
    void tcp_socket_read_some_error_handler(
            const boost::system::error_code& error);

    // 而 write 则可能是分几次调用 write_some, 如果中间有一次 write_some 失败，
    // 则 bytes_transferred 就会小于 期望写入的字节数
    // 所以 tcp_socket_write_error_handler 需要 bytes_transferred
    // on_write_error

    // 重载后无法解决std::bind 问题所以还是命名不同名字
    // write_error_handler_1
    template <typename ConstBufferSequence>
    void tcp_socket_write_error_handler_1 (
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const ConstBufferSequence&       __send_data) {

        // __send_data 有可能是 boost::array 或其他类型
        // 这将影响到 buffer_size 和 buffer_cast 的执行结果
        // 所以这里将其转换转制成 const_buffers_1
        auto&& send_data = asio::buffer(__send_data);
        std::size_t send_data_size = asio::buffer_size(send_data);
        // 未写完的数据的起始地址
        const_byte_ptr rest_data_ptr 
            = asio::buffer_cast<const_byte_ptr>(send_data) + bytes_transferred;
        auto self(shared_from_this());
        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                send_data_size,
                [&, self] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write(
                        asio::buffer (
                            rest_data_ptr,
                            send_data_size - bytes_transferred),
                        std::bind(&Connection::write_handler, 
                            shared_from_this(),
                            std::placeholders::_1));
                },
                // 未写完的数据的起始地址
                rest_data_ptr
        ); // __tcp_socket_write_error_wrapper
    }

    // write_error_handler_2
    void tcp_socket_write_error_handler_2 (
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const shared_data_type&          send_data) {
            
        auto self(shared_from_this());
        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                send_data->size(),
                [&, self] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write (
                        make_shared_data({
                                send_data->begin() + bytes_transferred,
                                send_data->end()
                        }),
                        std::bind(&Connection::write_handler, 
                            shared_from_this(),
                            std::placeholders::_1));
                },
                // 未写完的数据的起始地址
                &(*send_data)[0] + bytes_transferred
        ); // __tcp_socket_write_error_wrapper
    }
    // write_error_handler_3
    void tcp_socket_write_error_handler_3 (
            const boost::system::error_code& error,
            std::size_t                      bytes_transferred,
            const shared_reply_type&         send_data) {
            
        vdata_t&& origin_data = get_vdata_from_packet(*send_data);
        auto self(shared_from_this());
        __tcp_socket_write_error_wrapper (
                error, 
                bytes_transferred, 
                origin_data.size(),
                [&, self] (void) {
                    // 继续写入剩下的数据
                    tcp_socket_async_write (
                        make_shared_data(
                                origin_data.begin() + bytes_transferred,
                                origin_data.end()
                        ),
                        std::bind(&Connection::write_handler,
                            shared_from_this(), std::placeholders::_1));
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
            (bytes_transferred < data_size)                  &&
            (error != boost::asio::error::eof)               && 
            (error != boost::asio::error::connection_reset)  &&
            (error != boost::asio::error::operation_aborted) &&
            (m_tcp_socket.is_open())) {
            
            std::ostringstream oss;
            oss << "Data is not completely written ("
                << bytes_transferred << " bytes have been written last time). "
                "but the connection is still available. Continue to write...";
            call_logwarn(std::move(oss));
            // 继续写入剩下的数据
            wrf();
        }
        else {

            // 未写完的数据, 反馈给使用者
            // {
            // const_byte_ptr rest_data_ptr,
            // size_t         data_size - bytes_transferred
            // }
            // TODO
            (void)rest_data_ptr;

            std::ostringstream oss;
            oss << "write message error. " << error.message() 
                    << ", value=" << error.value() 
                    << ", (" << bytes_transferred 
                    << " bytes have been written last time). "
                    << "cancel this, this=" << this;
            call_logwarn(std::move(oss));
            // close
            tcp_socket_read_or_write_error_handler(error);
        }
    }

private:
    // tcp_socket_read_or_write_error_handler
    void tcp_socket_read_or_write_error_handler(
            const boost::system::error_code& error);

protected:
    /////////////////////////////////////////////
    // 封装异步读函数
    /////////////////////////////////////////////
    /*
    this->tcp_socket_async_read_some(buffer, 
            boost::bind(&Connection::read_handler, shared_from_this(),
                _1, e_request, data_client_rest));
    this->tcp_socket_async_read_some(buffer, 
            boost::bind(&Connection::read_handler, shared_from_this(),
                _1, e_request, data_client_rest),
            boost::bind(&Connection::read_or_write_error_handler, _1));
    auto self(shared_from_this());
    this->tcp_socket_async_read_some(buffer,
        [this, self] (size_t bytes_transferred) {});

    // Server 端是 shared_request_type, Client 端是 Shared_reply_type
    shared_requset_type shared_request;
    tcp_socket_async_read_some(shared_request,
        [this, self] (size_t bytes_transferred) {});
    */

    // 必须保证直到读取结束,buffer都存在于内存中
    // 重载一, 默认参数 ReadErrorHandler
    template<typename MutableBufferSequence, typename ReadSucceededHandler>
    void tcp_socket_async_read_some(
            const MutableBufferSequence& buffer,
            ReadSucceededHandler         succeeded_handler) {
        return tcp_socket_async_read_some(
                buffer, 
                succeeded_handler,
                std::bind(
                    &Connection::tcp_socket_read_some_error_handler,
                    shared_from_this(), 
                    std::placeholders::_1 // boost::system::error_code
                ));
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
                        shared_from_this(), _1, _2, 
                        succeeded_handler,
                        error_handler
                    ) // boost::bind
                ) // m_strand.wrap
        ); // this->m_tcp_socket.async_read_some

        // 执行 异步心跳循环
        //auto self(shared_from_this());
        //do_heartbeat_loop(self);
    }
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
        auto self(shared_from_this());
        do_heartbeat_loop(self);
        if (error) {
            // read error
            error_handler(error);
            return;
        }

        // 读取成功
        succeeded_handler(bytes_transferred);
    }


protected:
    /////////////////////////////////////////////
    // 封装异步写函数
    /////////////////////////////////////////////
    /*
    以下几个语句调用都是合法的
    tcp_socket_async_write(Server::pack_bad().buffers());
    auto self(shared_from_this());
    tcp_socket_async_write(Server::pack_bad().buffers(), [this,self](size_t){});
    tcp_socket_async_write(Server::pack_bad().buffers(), 
        std::bind(&Connetion::anyfunction, shared_from_this()));

    data_t data;
    tcp_socket_async_write(make_shared_data(std::move(data)), 
            [this, self] (size_t) {}
    );
    // server 端 shared_reply_type
    shared_reply_type reply_data;
    tcp_socket_async_write(reply_data), 
            [this, self] (size_t) {}
    );
    // Client 端是 shared_request_type

    tcp_socket_async_write(
        // buffer or shared_data
        buffer, 
    
        // SucceededHandler
        [this, self] (
            std::size_t bytes_transferred
        ) {
            // succeeded
            .....
        }
    );
    tcp_socket_async_write(
        // buffer
        buffer, 
    
        // SucceededHandler
        [this, self] (
            std::size_t bytes_transferred
        ) {
            // succeeded
            .....
        },
        
        // ErrorHandler, 自定义 error_handler
        [this, self] (
            const boost::system::error_code& error,
            std::size_t bytes_transferred,
            boost::asio::const_buffers_1 send_buff
        ) {
            // ...
        }
    );
     */


    // 重载一
    // 第一个参数是 buffer, 调用时必须保证 buffer 的数据直到回调仍然在内存中
    // 重载 1.1 默认参数 WriteSucceededHandler 和 WriteErrorHandler
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
                      <ConstBufferSequence>,
                      shared_from_this(),
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3)
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
            WriteErrorHandler          error_handler
        ) {

        auto self(shared_from_this());
        __write_wrapper ( 
                [&, self] (void) {
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
                                    shared_from_this(), _1, _2,
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
                std::bind(&Connection::tcp_socket_write_error_handler_2,
                        shared_from_this(),
                        std::placeholders::_1, // error_code
                        std::placeholders::_2, // bytes_transferred
                        std::placeholders::_3)// shared_data
                );
    }
    // 重载2.3
    template <typename WriteSucceededHandler, typename WriteErrorHandler>
    void tcp_socket_async_write(
            const shared_data_type& shared_data,
            WriteSucceededHandler   succeeded_handler,
            WriteErrorHandler       error_handler) {

        auto self(shared_from_this());
        __write_wrapper ( 
                [&, self] (void) {
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
                                    shared_from_this(), _1, _2,
                                    shared_data,
                                    succeeded_handler,
                                    error_handler
                                ) // boost::bind
                            ) // m_strand.wrap
                    ); // boost::asio::async_write
        }); // __write_wrapper
    }
    // 重载三
    // 第一个参数是 shared_reply_type 
    // 重载 3.1
    void tcp_socket_async_write(const shared_reply_type& shared_data) {
        return tcp_socket_async_write(shared_data, [] (std::size_t) {});        
    }
    // 重载 3.2
    template <typename WriteSucceededHandler>
    void tcp_socket_async_write(
            const shared_reply_type& shared_data,
            WriteSucceededHandler    succeeded_handler) {
        return tcp_socket_async_write(shared_data, succeeded_handler,
                std::bind(&Connection::tcp_socket_write_error_handler_3,
                        shared_from_this(),
                        std::placeholders::_1, // error_code
                        std::placeholders::_2, // bytes_transferred
                        std::placeholders::_3) // shared_data
                );
    }
    // 重载 3.3
    template <typename WriteSucceededHandler, typename WriteErrorHandler>
    void tcp_socket_async_write(
            const shared_reply_type& shared_data,
            WriteSucceededHandler    succeeded_handler,
            WriteErrorHandler        error_handler) {
        auto self(shared_from_this());
        __write_wrapper ( 
                [&, self] (void) {
                    boost::asio::async_write (
                            this->m_tcp_socket, 
                            shared_data->buffers(),
                            m_strand.wrap (
                                boost::bind (
                                    &Connection::tcp_socket_async_write_handler
                                    <shared_reply_type,
                                    //ReadSucceededHandler, ReadErrorHandler>,
                                    decltype(succeeded_handler),
                                    decltype(error_handler)>,
                                    shared_from_this(), _1, _2,
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
        //auto self(shared_from_this());
        //do_heartbeat_loop(self);
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
        auto self(shared_from_this());
        do_heartbeat_loop(self);
        if (error) {
            // write error
            error_handler(error, bytes_transferred, buffer_or_shared_data);
            return;
        }

        // 写入成功
        succeeded_handler(bytes_transferred);
    }

private:
    inline shared_request_type make_shared_request(
            std::size_t length = readbuffer::length_handshake) {
        //return std::make_shared<request>(readbuffer::max_length);
        return std::make_shared<request>(length);
    }
    inline shared_reply_type make_shared_reply(const reply& e_reply) {
        return std::make_shared<reply>(std::move(e_reply));
    }
private:
    static void call_logwarn(std::ostringstream&& oss);
protected:
    io_service::strand  m_strand;
    tcp::socket         m_tcp_socket;
    // 因为 在io.run 多线程下，最好不要 同时有多个 async_write 在异步执行 
    // 所以需要写锁
    UseStdMutex<true>   m_tcp_socket_write_mutex; 
    // udp::socket      m_udp_socket;
    Heartbeat::Receiver m_heartbeat; // m_heartbeat 依赖 m_strand, 不可调换
    shared_session_t    m_session = nullptr;
private:
    ConnetionStatus::status_t m_status;
private:
    static Engine::test_and_set_value<bool>  m_write_in_multithreads;
    // 只有当 m_tcp_socket_write_mutex 的类型为 UseStdMutex<true>
    // 且 m_write_in_multithreads 的值为 true 时, 才会开启锁的功能
}; // class Engine::Server::Connection

} // namespace Server
} // namespace Engine

#endif // E_CONNECTION_H_1

