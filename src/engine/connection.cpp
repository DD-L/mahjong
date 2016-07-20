/*************************************************************************
	> File Name:    connection.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/17 14:30:52
 ************************************************************************/

#include <engine/connection.h>
#include <engine/log.h>
using namespace Engine;


void Connection::start(void) throw (ConnectionException) {
    boost::system::error_code ec; 
    loginfo("client: " << m_tcp_socket.remote_endpoint(ec).address()
            << ", this=" << this);
    if (ec) {
        logerror(ec.message() << ", value=" << ec.value()
                << ". Terminate this Connection!!! this=" << this);
        throw ConnectionException(ec.message());
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
    auto&& e_request = make_shared_request(readbuffer::max_length);
    m_tcp_socket.async_read_some(e_request->buffers(),
            boost::bind(&Connection::read_handler, shared_from_this(),
                _1, _2, e_request,
                Engine::placeholders::shared_data,
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
        shared_data_type __data_rest, shared_data_type __write_data) {
    
//    (void)__write_data;
//    if (__data_rest->size() > 0) {
//        // 先修正 byte_transferred
//        // bytes_transferred += 上一次的bytes_transferred
//        bytes_transferred += 
//            e_request->get_data().size() + packet::header_length;
//        // 分包处理后，遗留的数据
//        // append 追加数据__data_rest
//        // 修正 e_request
//        e_request->get_data().insert(e_request->get_data().end(),
//                __data_rest->begin(), __data_rest->end());
//        elogdebug("e_request was rectified");
//    }
//
//    elogdebug("---> bytes_transferred = " << std::dec << bytes_transferred);
//    elogdebug("e_request->version() = " << (int)e_request->version());
//    elogdebug("e_request->type() = " << (int)e_request->type());
//    elogdebug("e_request->data_len() = " << e_request->data_len());
//    elogdebug(_debug_format_data(get_vdata_from_packet(*e_request),
//            int(), ' ', std::hex));
}
