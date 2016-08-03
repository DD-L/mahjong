/*************************************************************************
	> File Name:    connection.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/17 14:30:52
 ************************************************************************/

#include <engine/connection.h>
#include <engine/log.h>

namespace Engine {
namespace Server {

using namespace Engine::ConnetionStatus; // for status_t

Connection::Connection(boost::asio::io_service& io_service) 
        : m_tcp_socket(io_service) {
    // TODO
    m_status = status_data;
}

Connection::~Connection() {
    this->close();    
}

void Connection::start(void) throw (Excpt::ConnectionException) {
    boost::system::error_code ec; 
    loginfo("client: " << m_tcp_socket.remote_endpoint(ec).address()
            << ", this=" << this);
    if (ec) {
        logerror(ec.message() << ", value=" << ec.value()
                << ". Terminate this Connection!!! this=" << this);
        throw Excpt::ConnectionException(ec.message());
    }

    // set keepalive
    boost::asio::socket_base::keep_alive option(true);
    m_tcp_socket.set_option(option, ec);
    if (ec) {
        logwarn(ec.message() << ", value=" << ec.value()
                << ", m_tcp_socket.set_option::keep_alive");
    }

    // start read message from client 
    elogdebug("begin to read message from client.");
    // ...
    //
    do_receive_message();
}

void Connection::do_receive_message(void) {
    auto&& e_request = make_shared_request(readbuffer::max_length);
    m_tcp_socket.async_read_some(e_request->buffers(),
            boost::bind(&Connection::read_handler, shared_from_this(),
                _1, _2, e_request,
                Engine::placeholders::shared_data));
}

void Connection::close(void) throw ()
try {
    boost::system::error_code ec;
    if (m_tcp_socket.is_open()) {
        m_tcp_socket.shutdown(tcp::socket::shutdown_both, ec);
        if (ec) {
            elogdebug(ec.message() << " value=" << ec.value()
                    << ", m_tcp_socket::shutdown, this=" << this);
        }
        m_tcp_socket.close(ec);
        if (ec) {
            elogdebug(ec.message() << " value=" << ec.value()
                    << ", m_tcp_socket::close, this=" << this);
        }
    }
}
catch (boost::system::system_error const& e) {
    logerror(e.what());
}
catch (std::exception& e) {
    logerror(e.what());
}
catch (...) {
    logerror("An error has occurred");
}


// 调用 cancel 会引发 异步操作立刻返回 error::operation_aborted, 
// 进而Connection 的引用计数自然终结至 0 , 最终 close socket
// 服务端将认为是客户端的非法访问而导致异常或错误，所以可以放心的 cancel
void Connection::cancel(void) throw () 
try {
    boost::system::error_code ec;
    m_tcp_socket.cancel(ec);
    if (ec) {
        elogdebug(ec.message() << " value=" << ec.value()
                << ", m_m_tcp_socket::cancel, this=" << this);
    }
}
catch (boost::system::system_error const& e) {
    logerror(e.what());
}
catch (std::exception& e) {
    logerror(e.what());
}
catch (...) {
    logerror("An error has occurred");
}

void Connection::read_handler(const boost::system::error_code& error,
        std::size_t bytes_transferred, shared_request_type e_request,
        shared_data_type __data_rest) {
    
    if (__data_rest->size() > 0) {
        // 先修正 byte_transferred
        // bytes_transferred += 上一次的bytes_transferred
        bytes_transferred += 
            e_request->get_data().size() + packet::header_size;
        // 分包处理后，遗留的数据
        // append 追加数据__data_rest
        // 修正 e_request
        e_request->get_data().insert(e_request->get_data().end(),
                __data_rest->begin(), __data_rest->end());
        elogdebug("e_request was rectified");
    }

    elogdebug("---> bytes_transferred = " << std::dec << bytes_transferred);
    elogdebug("e_request->version() = " << (int)e_request->version());
    elogdebug("e_request->type() = " << (int)e_request->type());
    elogdebug("e_request->data_len() = " << e_request->data_len());
    elogdebug(_debug_format_data(get_vdata_from_packet(*e_request),
            int(), ' ', std::hex));

    if (error) {
        logwarn(error.message() << ", value=" << error.value() 
                << ". cancel this, this=" << this);
        this->cancel();
        return;
    }
    // success
    try {
///////////////////////////////////////////////
// 分析包数据
///////////////////////////////////////////////
switch (e_request->version()) {
case 0x00: // packet.version
{
    bool is_zip_data = false;

    // e 包完整性检查
    Engine::packet_integrity_check(bytes_transferred, *e_request);

    switch (e_request->type()) {
    // TODO
    case request::hello:
    case request::exchange:
        break;
    case request::zipdata:
        is_zip_data = true;
    case request::data:
    {
        assert_status(status_data);
        // step 1
        // 解包 得到 plain_data
        data_t plain_data;
        const int rest_e_data_len = unpack_data(
                plain_data, bytes_transferred,
                *e_request, is_zip_data);

        if (rest_e_data_len < 0) {
            throw Excpt::incomplete_data(0 - rest_e_data_len);
        }

        elogdebug("unpack data from client: " <<
                _debug_format_data(plain_data, int(), ' ', std::hex));

        // step 2
        // 分包， 裁剪 e_request 数据，（当前数据已缓存到* plain_data_ptr） 
        auto&& plain_data_ptr = Engine::make_shared_data(std::move(plain_data));
        bool is_continue =
            Engine::cut_e_pack(bytes_transferred - rest_e_data_len,
                    bytes_transferred, *e_request);
        // bytes_transferred - rest_e_data_len 的意义是
        // 新包在 旧包中 开始的位置（0开头）
        
        // 将  plain_data_ptr 交给 Session 或 子类处理
        if (rest_e_data_len > 0 && is_continue) {
            // e_request 里还有未处理的数据
            elogdebug("unprocessed data still in e_request..");
            // 

            // TODO

            //将 plain_data_ptr 交给 Session 或 子类处理，然后回调还 绑定自己
           // boost::bind(&Connection::read_handler, 
           //             shared_from_this(), 
           //             boost::asio::placeholders::error,
           //             std::size_t(rest_e_data_len), 
           //             e_request, 
           //             Engine::placeholders::shared_data);


           // 最后 递归自己
           this->read_handler(error, std::size_t(rest_e_data_len),
                   e_request, Engine::placeholders::shared_data);
        }
        else {
            // 最后一条不可再分割的数据再绑定 写
            // 将 plain_data_ptr 交给 Session 或 子类处理，然后回调 绑定 谁，
            // 由  Session 或 子类 处理 
            // TODO

            // 循环读
            do_receive_message();
        }
         
    } // end - request::data
        break;
    case request::bad:
    default:
        // 数据包 bad
        logwarn("e_packet is bad. cancel this, this=" << this);
        this->cancel();
        return;
        break;
    } // end - switch (e_request->type())
} // end -  e_request->version() == 0x00 
    break;
default: 
    logerror("Unkown packet version");
    break;
} // end - switch (e_request->version())
///////////////////////////////////////////////
///////////////////////////////////////////////
    } // try {}
    catch (Excpt::wrong_packet_type const&) {
        // TODO
        logwarn("wrong_packet_type. cancel this, this=" << this);
        this->cancel();
        return;
    }
    catch (Excpt::incomplete_data const& ec) {
        // 不完整数据
        // 少了 ec.less() 字节
        logwarn("incomplete_data. ec.less() = " << ec.less()
                << " byte. this=" << this);
        if (ec.less() > 0) {
            auto&& data_client_rest = Engine::make_shared_data(
                    (std::size_t)ec.less(), 0);

            elogdebug("incomplete_data. start to async-read " 
                    << ec.less() << " byte data from m_tcp_socket");

            const std::size_t e_pack_size =
                e_request->get_data().size() + packet::header_size; 

            if (bytes_transferred < e_pack_size) {
                // 当前包数据不完整，需要再读一些
                // 修正 e_request 中的 data 字段的 size
                e_request->get_data().resize(
                        bytes_transferred - packet::header_size);
            }
            else if (bytes_transferred == e_pack_size) {
                // 分包后，读取的远程数据不完整，还有遗留未读数据
            }
            else {
                // 理论上不可能出现这种情况
                _print_s_err("impossible!! "<< __FILE__<< ":" << __LINE__);
            }
            
            this->m_tcp_socket.async_read_some(
                    boost::asio::buffer(&(*data_client_rest)[0],
                        (std::size_t)ec.less()),
                    boost::bind(&Connection::read_handler,
                        shared_from_this(), _1, _2, e_request,
                        data_client_rest));
        }
        else {
            logwarn("send engine_bad to local. finally cancel this, this="
                    << this);
            boost::asio::async_write(this->m_tcp_socket,
                    Server::pack_bad().buffers(),
                    boost::bind(&Connection::cancel, shared_from_this()));
        }
    }
    catch (Excpt::wrong_conn_status const& ec) {
        logwarn(ec.what() << ". cancel this, this=" << this);
        this->cancel();
        return; 
    }
    catch (Excpt::EncryptException const& ec) {
        logwarn(ec.what() << ". cancel this, this=" << this);
        this->cancel();
        return;
    }
    catch (Excpt::DecryptException const& ec) {
        logwarn(ec.what() << ". cancel this, this=" << this);
        this->cancel();
        return;
    }
    catch (std::exception const& ec) {
        logwarn(ec.what() << ". cancel this, this=" << this);
        this->cancel();
        return;
    }
    catch (...) {
        logwarn("cancel this, this=" << this);
        this->cancel();
        return;
    }
} // end - function Connection::read_handler


/**
 * @brief unpack_data
 * @param plain [out]
 * @param e_pack_length  当前尚未解包的 e_pack 数据长度, 
 *                      总是传入 bytes_trannsferred
 * @param is_zip [bool, default false]
 * @return [const int]     这次解包后（unpack 执行完），还剩
 *                          未处理的 e_request 数据长度. 
 *      = 0 数据已处理完毕
 *      > 0 e_request 中还有未处理完的数据
 *      < 0 当前 e_request 数据包不完整
 */
const int Connection::unpack_data(data_t& plain, 
        const std::size_t e_pack_length,
        const request& request, bool is_zip/* = false*/) {
    // TODO
    //if (! this->m_aes_encryptor) {
    //    // this->m_aes_encryptor 未被赋值
    //    logwarn("aes encrytor is not initialized, send lss_deny to client, "
    //            "finally cancel this, this=" << this);
    //    // 发送 deny. 并 cancel this
    //    boost::asio::async_write(this->m_tcp_socket, pack_deny().buffers(),
    //            boost::bind(&Connection::canncel, shared_from_this()));
    //}

    const data_t&     cipher     = request.get_data();
    const std::size_t cipher_len = request.data_len();

    if (is_zip) {
        // TODO
    }

    // TODO
    // 对包解密
    //vdata_t plain_;
    //this->m_aes_encryptor->decrypt(plain_, &cipher[0], cipher_len);
    
    // 临时处理
    plain.assign(cipher.begin(), cipher.begin() + cipher_len);
    // TODO
    //plain.assign(plain_.begin(), plain_.end());
    return e_pack_length - (packet::header_size + cipher_len);
    //  (packet::header_size + cipher_len) 当前已经处理的 e_pack 包长度

} // end -  function Connection::unpack_data


} // namespace Engine::Server
} // namespace Engine
