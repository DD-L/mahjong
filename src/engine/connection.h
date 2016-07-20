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
#include <engine/sessionmanager.h>

namespace Engine {

// 负责网络通信协议部分
class Connection : public std::enable_shared_from_this<Connection> { 
private:
    using request = Engine::Server::request;
    using reply   = Engine::Server::reply;
public:
    typedef std::shared_ptr<Connection> pointer;
    typedef Connection                  BASETYPE;
    typedef std::shared_ptr<request> shared_request_type;
    typedef std::shared_ptr<reply>   shared_reply_type;
public:
    virtual ~Connection() {}
    Connection(const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;
    void start(void) throw (ConnectionException);
    virtual void close(void) throw ();
    virtual void cancel(void) throw (); 

    inline tcp::socket& tcp_socket() {
        return m_tcp_socket; 
    }
protected:
    explicit Connection(boost::asio::io_service& io_service) 
        : m_tcp_socket(io_service) {}
private:
    void read_handler(const boost::system::error_code& error,
            std::size_t bytes_transferred, shared_request_type e_request,
            shared_data_type __data_rest, shared_data_type __write_data);
private:
    inline shared_request_type make_shared_request(
            std::size_t length = readbuffer::length_handshake) {
        //return std::make_shared<request>(readbuffer::max_length);
        return std::make_shared<request>(length);
    }
    inline shared_reply_type make_shared_reply(const reply& lss_reply) {
        return std::make_shared<reply>(std::move(lss_reply));
    }
protected:
    tcp::socket  m_tcp_socket;
    // udp::socket m_udp_socket;
private:
    std::shared_ptr<Session> m_session;
}; // class Connection

} // namespace Engine

#endif // E_CONNECTION_H_1

