/*************************************************************************
	> File Name:    src/client/connection.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/24 11:24:39
 ************************************************************************/

#include <client/connection.h>
#include <client/config.h>

// for Engine::Client

namespace Engine {
namespace Log {
static inline sdata_t basename(const sdata_t& filename) {
    // http://www.cplusplus.com/reference/string/string/find_last_of/
    const std::size_t found = filename.find_last_of("/\\");
    return filename.substr(found + 1);
}
}} // namespace Engine::Log

//#ifdef ENGINE_DEBUG
#include <iostream>
#include <sstream>

    #ifdef elogdebug
        #undef elogdebug
    #endif

    #define elogdebug(msg) \
        { std::ostringstream oss;\
            oss << msg \
                << '\t' << __func__ \
                << ' ' << Engine::Log::basename(__FILE__) << ':' << __LINE__ ;\
            Engine::Client::Connection::DebugMessage(std::move(oss.str())); \
        } while (0)
//#else
//    #ifdef elogdebug
//        #undef elogdebug
//    #endif
//
//    #define elogdebug(msg)
//#endif

namespace Engine {
namespace Client {


Engine::test_and_set_value<Engine::uuid_t> Connection::m_uuid;
std::mutex Connection::_debug_message_callback_mutex;
std::function<void(const char*)> Connection::_debug_message_callback;
Engine::test_and_set_value<bool> Connection::m_write_in_multithreads(false);

Connection::Connection(io_service& io_service) 
        : m_strand(io_service),
          m_heartbeat(m_strand,
                  Config::get_instance().get_ping_interval()
          ),
          m_resolver(io_service), 
          m_tcp_socket(io_service) {
    get_io_service().stop();
}


Connection::~Connection() { 
    this->stop(); 
}

void Connection::set_ping_interval(uint16_t interval) {
    m_heartbeat.set_default_interval(interval);
}

void Connection::run(CallbackConstCharPtr callback) {
    std::lock_guard<std::mutex> lock(m_run_mutex);
    if (! get_io_service().stopped()) {
        callback("Already running");
        return;
    }
    // 复用 m_io_thread
    this->stop();
    std::thread t(std::bind(&Connection::thread_working, this, callback));
    this->m_io_thread = std::move(t);
    // 等待线程启动
    while (get_io_service().stopped()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
    }
}

void Connection::start(const sdata_t& server_name, 
        const sdata_t& server_port, 
        CallbackInt callback) {
    if (this->m_tcp_socket.is_open()) {
        callback(-1);
        return;
    }
    m_resolver.async_resolve({server_name, server_port},
            boost::bind(
                &Connection::resolve_handler, this, _1, _2, callback)
            );
}

void Connection::stop() {
    get_io_service().stop();
    try {
        if (this->m_io_thread.joinable()) {
            this->m_io_thread.join();
        }
        else {
            this->m_io_thread.detach();
        }
    }
    catch (const std::system_error&) {}
    catch (...) {}
}


void Connection::thread_working(CallbackConstCharPtr callback) {
    io_service::work work(get_io_service());
    get_io_service().reset();
    for (;;) {
        try {
            get_io_service().run();
            break;
        }
        catch (boost::system::system_error const& e) {
            callback(e.what());
        }
        catch (const std::exception& e) {
            callback(e.what());
        }
        catch (...) {
            callback("An error has occurred. io_service.run()");
        }
    }
    ///
    callback("Stopped running");
}


void Connection::cancel_socket() throw()
try {
    boost::system::error_code ec;
    m_heartbeat.cancel(ec);
    if (ec) {
        (void)ec.message();
        (void)ec.value(); 
    }
    m_tcp_socket.cancel(ec);
    if (ec) {
        (void)ec.message();
        (void)ec.value(); 
    }
}
catch (boost::system::system_error const&) {}
catch (std::exception&) {}
catch (...) {}


void Connection::close_socket(bool exec_callback/* = false*/) throw()
try {

    cancel_socket();

    if (m_tcp_socket.is_open()) {
        boost::system::error_code ec;
        m_tcp_socket.shutdown(tcp::socket::shutdown_both, ec);
        if (ec) {
            (void)ec.message();
            (void)ec.value(); 
        }

        m_tcp_socket.close(ec);
        if (ec) {
            (void)ec.message();
            (void)ec.value();
        }


        // 断线回调
        if (exec_callback && m_disconnect_callback) {
            boost::shared_lock<boost::shared_mutex> 
                lock(m_disconnect_callback_mutex);
            m_disconnect_callback();
        }
    }

}
catch (boost::system::system_error const&) {}
catch (std::exception&) {}
catch (...) {}


void Connection::resolve_handler(const boost::system::error_code& ec,
        tcp::resolver::iterator i, CallbackInt callback) {
    if (! ec) {
        boost::asio::async_connect(m_tcp_socket, i,
            boost::bind(&Connection::connect_handler, 
                this, _1, _2, callback));
    } 
    else {
        // 主机不可达
        close_socket();
        callback(1);   
        return;
    }
}


const_byte_ptr Connection::uuid_data(void) {
        for (;;) {
            try {
                (void)m_uuid.value();
                break;
            }
            catch (std::exception const&) {
                m_uuid.set(Engine::uuid_t());
            }
        }
        return m_uuid.value().data;
}

data_t Connection::ping_interval_binary(void) {
    uint16_t interval = m_heartbeat.defualt_interval();
    byte d[2];
    d[0] = (interval >> 8) & 0xff; // 高字节
    d[1] = interval & 0xff;        // 低字节
    return data_t(d, d + 2);
} 


void Connection::connect_handler(const boost::system::error_code& ec,
        tcp::resolver::iterator i, CallbackInt callback) {
    if (! ec) {
        // 连接 Server 成功

        // 发送 iddata
        const_byte_ptr ud = uuid_data();
        data_t iddata(ud, ud + Engine::uuid_t::static_size());
        iddata += ping_interval_binary();
        // 包数据组成：18个字节： 前 16字节 uuid, 后 2个字节 ping_interval
        auto&& shared_iddata 
            = make_shared_request(pack_iddata(std::move(iddata)));
        tcp_socket_async_write(shared_iddata, 
                [this, callback](std::size_t) {
                    // 启动 Receive Message Loop
                    do_receive_message();

                    // Server 就绪
                    callback(0);
                });
    }
    else if (i != tcp::resolver::iterator()) {
        this->m_tcp_socket.close();
        tcp::endpoint endpoint = *i;
        this->m_tcp_socket.async_connect(endpoint,
                boost::bind(&Connection::connect_handler, this,
                    boost::asio::placeholders::error, ++i, callback));
    }
    else {
        // 网络不可达
        close_socket();
        callback(2);
        return;
    }
}  


// write
void Connection::send_message(data_t&& data, CallbackSize callback) {
    // step 1 加密
    // step 2 压缩
    // TODO

    // step 3 发送
    auto&& data_request = make_shared_request(pack_data(std::move(data)));
    tcp_socket_async_write(data_request, 
            std::bind(callback, std::placeholders::_1));
}


void Connection::do_receive_message(void) {
    auto&& e_reply = make_shared_reply(readbuffer::max_length);
    /*
    this->m_tcp_socket.async_read_some(e_reply->buffers(),
            boost::bind(&Connection::read_handler, this,
                _1, _2, e_reply, Engine::placeholders::shared_data));
                */
    tcp_socket_async_read_some(e_reply->buffers(),
            std::bind(&Connection::read_handler, this,
                std::placeholders::_1, e_reply,
                Engine::placeholders::shared_data));

}
void Connection::read_handler(
            std::size_t       bytes_transferred, 
            shared_reply_type e_reply,
            shared_data_type  __data_rest) {
    if (__data_rest->size() > 0) {
        // 先修正 bytes_transferred
        // bytes_transferred += 上一次的bytes_transferred
        bytes_transferred += 
            e_reply->get_data().size() + packet::header_size;

        // 分包处理后，遗留的数据
        // append 追加数据__data_rest
        // 修正 e_reply
        e_reply->get_data().insert(e_reply->get_data().end(),
                __data_rest->begin(), __data_rest->end());
        elogdebug("e_reply was rectified");
    }

    elogdebug("---> bytes_transferred = " << std::dec << bytes_transferred);
    elogdebug("e_reply->version() = " << (int)e_reply->version());
    elogdebug("e_reply->type() = " << (int)e_reply->type());
    elogdebug("e_reply->data_len() = " << e_reply->data_len());
    elogdebug(_format_data(get_vdata_from_packet(*e_reply),
            int(), ' ', std::hex));

    try {
///////////////////////////////////////////////
// 分析包数据
///////////////////////////////////////////////
switch (e_reply->version()) {
case 0x00: {
    bool is_zip_data = false;

    // e 包完整性检查
    Engine::packet_integrity_check(bytes_transferred, *e_reply);

    switch (e_reply->type()) {
    // TODO
    case reply::hello:
    case reply::exchange:
        do_receive_message();
        return;
        break;
    case reply::zipdata:
        is_zip_data = true;
    case reply::data: {
        //assert_status(status_data);
        // step 1
        // 解包 得到 plain_data
        data_t plain_data;
        const int rest_e_data_len = unpack_data(
                plain_data, bytes_transferred,
                *e_reply, is_zip_data);

        if (rest_e_data_len < 0) {
            throw Excpt::incomplete_data(0 - rest_e_data_len);
        }

        elogdebug("unpack data from server: " <<
                _debug_format_data(plain_data, int(), ' ', std::hex));

        // step 2
        // 分包， 裁剪 e_reply 数据（当前数据已缓存到 *plain_data_ptr）
        auto&& plain_data_ptr = Engine::make_shared_data(std::move(plain_data));
        bool is_continue =
            Engine::cut_e_pack(bytes_transferred - rest_e_data_len,
                    bytes_transferred, *e_reply);
        // byte_transferred - rest_e_data_len 的意义是
        // 新包在旧包中 开始的位置 (0开头)
        
        // 将 plain_data_ptr 交给 回调处理
        if (rest_e_data_len > 0 && is_continue) {
            // e_reply 里还有未处理的数据
            elogdebug("unprocessed data still in e_reply..");

            // TODO
            // 将 plain_data_ptr 交给 回调处理，然后 递归自己
            //
            call_received_callback((const char*) (plain_data_ptr->c_str()),
                    (std::size_t) (plain_data_ptr->length()));

            this->read_handler(std::size_t(rest_e_data_len),
                    e_reply, Engine::placeholders::shared_data);
        }
        else {
            // 最后一条不可再分割的数据再绑定 读
            // 将 plain_data_tr 交给 回调处理，然后该绑定谁就绑定谁

            call_received_callback((const char*) (plain_data_ptr->c_str()),
                    (std::size_t) (plain_data_ptr->length()));

            // 循环读
            do_receive_message();
        }  
    } // end - reply::data
        break;
    case reply::deny:
        // 被 server 拒绝
        elogdebug("deny");
        // TODO
        do_receive_message();
        return;
    case reply::timeout:
        // server 端 处理超时
        elogdebug("timeout");
        do_receive_message();
        return;
    //case Engine::Server::request::ping: // 来自 Server 端的 ping
    //case reply::ping: // 来自 Server 端的 ping
    //    respond_server_ping(); 
    //    break;
    case reply::bad:
    default:
        elogdebug("e_pakcet is bad.");
        return;
        break;
    } // switch e_reply->type
}
break;
default:
    elogdebug("Unkown packet version");
    break;
} // switch e_reply->version    
        // ........
    } // try {}
    catch (Excpt::wrong_packet_type const&) {
        // TODO
        elogdebug("wrong_packet_type. cancel this, this=" << this);
        return;
    }
    catch (Excpt::incomplete_data const& ec) {
        // 不完整数据
        // 少了 ec.less() 字节
        elogdebug("incomplete_data. ec.less() = " << ec.less()
                << " byte. this=" << this);
        if (ec.less() > 0) {
            auto&& data_client_rest = Engine::make_shared_data(
                    (std::size_t)ec.less(), 0);

            elogdebug("incomplete_data. start to async-read " 
                    << ec.less() << " byte data from m_tcp_socket");

            const std::size_t e_pack_size =
                e_reply->get_data().size() + packet::header_size; 

            if (bytes_transferred < e_pack_size) {
                // 当前包数据不完整，需要再读一些
                // 修正 e_reply 中的 data 字段的 size
                e_reply->get_data().resize(
                        bytes_transferred - packet::header_size);
            }
            else if (bytes_transferred == e_pack_size) {
                // 分包后，读取的远程数据不完整，还有遗留未读数据
            }
            else {
                // 理论上不可能出现这种情况
                elogdebug("impossible!! ");
            }
            
            // 再读一些
            //this->m_tcp_socket.async_read_some(
            //        boost::asio::buffer(&(*data_client_rest)[0],
            //            (std::size_t)ec.less()),
            //        boost::bind(&Connection::read_handler,
            //            this, _1, _2, e_reply,
            //            data_client_rest));
            tcp_socket_async_read_some(
                    asio::buffer(
                        &(*data_client_rest)[0],
                        (std::size_t)ec.less()
                    ), // buffer
                    std::bind(&Connection::read_handler, this,
                        std::placeholders::_1,
                        e_reply,
                        data_client_rest
                    ) // std::bind
            );
        }
        else { // ec.less() <= 0
            //elogdebug("send engine_bad to local. finally cancel this, this="
            //        << this);
            //boost::asio::async_write(this->m_tcp_socket,
            //        pack_bad().buffers(),
            //        boost::bind(&Connection::cancel, this));
            elogdebug("Excpt::incomplete_data::less() <= 0, value="<<ec.less());
            //TODO
            //TODO
            //TODO
        }
    }
    catch (Excpt::wrong_conn_status const& ec) {
        elogdebug(ec.what());
        return; 
    }
    catch (Excpt::EncryptException const& ec) {
        elogdebug(ec.what());
        return;
    }
    catch (Excpt::DecryptException const& ec) {
        elogdebug(ec.what());
        return;
    }
    catch (std::exception const& ec) {
        elogdebug(ec.what());
        return;
    }
    catch (...) {
        elogdebug("An error occured while parsing connect-protocol.");
        return;
    }

//        {
//            // std::shared_lock 直到 C++14 才被支持
//            // 而 std::shared_mutex 直到 C++17 才被支持
//            //boost::shared_lock<boost::shared_mutex> lock(m_received_callback_mutex)
//            if (m_received_callback) {
//                m_received_callback_mutex.lock_shared();
//                m_received_callback((const char*) data, (size_t) len);
//                m_received_callback_mutex.unlock_shared();
//            }
//        }
} // end read_handler


/**
 * @brief unpack_data
 * @param plain [out]
 * @param e_pack_length  当前尚未解包的 e_pack 数据长度, 
 *                      总是传入 bytes_trannsferred
 * @param is_zip [bool, default false]
 * @return [const int]     这次解包后（unpack 执行完），还剩
 *                          未处理的 e_reply 数据长度. 
 *      = 0 数据已处理完毕
 *      > 0 e_reply 中还有未处理完的数据
 *      < 0 当前 e_reply 数据包不完整
 */
const int Connection::unpack_data(data_t& plain, 
        const std::size_t e_pack_length,
        const reply& reply, bool is_zip/* = false*/) {
    // TODO
    //if (! this->m_aes_encryptor) {
    //    // this->m_aes_encryptor 未被赋值
    //    this->aes_encryptor = std::make_shared<crypto::Encryptor>(
    //      new crypto::Aes(this->data_key, crypto::Aes::raw256keysetting())
    //    );
    //}

    const data_t&     cipher     = reply.get_data();
    const std::size_t cipher_len = reply.data_len();

    if (is_zip) {
        // TODO
        // 解压 data
    }

    // 对包解密
    // TODO
    //vdata_t plain_;
    //this->m_aes_encryptor->decrypt(plain_, &cipher[0], cipher_len);

    // 临时处理
    plain.assign(cipher.begin(), cipher.begin() + cipher_len);
    // TODO
    //plain.assign(plain_.begin(), plain_.end());
    return e_pack_length - (packet::header_size + cipher_len);
    //  (packet::header_size + cipher_len) 当前已经处理的 e_pack 包长度
} // end - function Connection::unpack_data



// 异步心跳循环，
// 正常情况下只有在 主动 heartbeat.cancel() 时 才会结束循环
void Connection::do_heartbeat_loop() {
    elogdebug("do_heartbeat_loop(). this=" << this); 
    this->m_heartbeat.async_wait(
            // 过期（未被取消）回调
            [this] (void) {
                elogdebug("heartbeat expired, send ping packet to server...");
                // send_ping to server
                tcp_socket_async_write(pack_ping().buffers());
                // loop 已经由上句的 tcp_socket_async_write 发出, 这里不必重复
                //do_heartbeat_loop();
            });
}

// on_read_error
void Connection::tcp_socket_read_some_error_handler(
        const boost::system::error_code& error) {
    close_socket(true);

    // 可自定义
    // read_error 回调
    if (m_read_error_callback) {
        m_read_error_callback(error.message().c_str(), error.value());
    }

    // boost::asio::error::eof == 2 
    // // 本地 socket 所在连接被对方完全关闭（4次握手） 会引发 eof
    //
    // boost::asio::error::operation_aborted == 995
    // // 本地socket的超时,cancel,close,shutdown 都会引发 operation_aborted
    //
    // boost::asio::error::bad_descriptor == 10009
    // // 在一个已经关闭了的套接字上执行 async_receive 或 async_read 
    //
    // boost::asio::error::connection_reset == 10054
    // // 正在async_receive()异步任务等待时，
    // 远端的TCP协议层发送RESET终止链接，暴力关闭套接字。
    // 常常发生于远端进程强制关闭时，操作系统释放套接字资源。区别于 eof
    //

    return;
} // end - function Connection::tcp_socket_read_some_error_handler


} // namespace Engine::Client
} // namespace Engine

