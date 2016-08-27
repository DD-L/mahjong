#ifndef E_SERVER_H_1
#define E_SERVER_H_1
/*************************************************************************
	> File Name:    server.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/14 11:34:56
 ************************************************************************/


#include <engine/event.h>
#include <engine/connection.h>
#include <engine/config_server.h>

///////////////////////////////////////////////
namespace Engine {
namespace Server {
    template<typename T>
    class Server;
};

template<typename T>
using EServer = Server::Server<T>;

} // namespace Engine


namespace Engine {

//////////////////////////////////////
// typedef Server::Server EServer;
//////////////////////////////////////

// namespace Engine::Server
namespace Server {

// class Engine::Server::Server 
template <typename CONNECTION>
class Server {
public:
    typedef CONNECTION connection_type;
    static_assert(std::is_base_of<Connection, connection_type>::value,
        "connection_type should be derived from 'Engine::Server::Connection'!");
public:
    Server(void) : 
        m_acceptor(get_io_service()),
        m_acceptor_io_thread_n (
            Engine::Server::Config::get_instance().get_acceptor_io_thread()  
        ) {};
    Server(const Server&) = delete;
    virtual ~Server() {
        if (m_event_thread.joinable()) {
            m_event_thread.join();
        }
    }
    void run(const sdata_t& bind_addr, const uint16_t bind_port) {

        // step 1 启动事件处理引擎
        std::thread t(std::bind(&Server::start_event, this));
        t.detach();
        m_event_thread = std::move(t);

        // step 2 启动 tcp 网络服务器
        try {
            ip::tcp::endpoint endpoint(
                    ip::address::from_string(bind_addr), bind_port);
            m_acceptor.open(endpoint.protocol());
            m_acceptor.set_option(socket_base::reuse_address(true));
            m_acceptor.bind(endpoint);
            m_acceptor.listen();
        }
        catch (boost::system::system_error const& e) {
            logerror(e.what());
            // 绑定本地端口失败，关闭事件处理引擎
            stop_event();
            return;
        }

        // 绑定端口成功，启动异步 accept
        start_accept();

        boost::asio::io_service::work work(m_acceptor.get_io_service());

        // 将是否是多线程的信息反馈给 Connection
        Connection::inital_multithread_setting(m_acceptor_io_thread_n);        

        // 在子线程中启动1个或多个 m_acceptor.get_io_service().run()
        std::vector<std::thread> vt;
        std::size_t thread_n = m_acceptor_io_thread_n;
        while (thread_n--) {
            vt.push_back(std::move(
                        std::thread(
                                std::bind(&Server::thread_working,
                                    this//,
                                    //std::ref(..)
                            ))));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        for (auto& v : vt) {
            v.join();
        }
        loginfo("Server::run() exit!");
    }

    void stop(void) {
         stop_event();
         stop_network();
    }
    void stop_event(void) {
        Event::get_instance().stop();
    }
    void stop_network(void) {
         m_acceptor.get_io_service().stop();
    }

    bool stopped(void) const {
        return event_stopped() && network_stopped();
    }
    bool event_stopped(void) const {
        return Event::get_instance().stopped();
    }
    bool network_stopped(void) const {
        return const_cast<Server*>(this)->m_acceptor.get_io_service().stopped();
    }

private:
    void start_event(void) {
        // 起n个线程用于事件处理驱动
        std::size_t thread_n
            = Engine::Server::Config::get_instance().get_event_io_thread();
        Event::get_instance().run(thread_n);
    }
    // 
    void thread_working(void) {
        for (;;) {
            try {
                m_acceptor.get_io_service().run();
                break;
            }
            catch (boost::system::system_error const& e) {
                logerror(e.what());
            }
            catch (const std::exception& e) {
                logerror(e.what());
            }
            catch (...) {
                logerror("An error has occurred. Server_ios.run()");
            }
        }
        loginfo("one of acceptor-threads exit!");
    }

private:
    void start_accept(void) {
        Connection::pointer new_connection = 
            std::make_shared<connection_type>(
                    m_acceptor.get_io_service()
            );
        m_acceptor.async_accept(new_connection->tcp_socket(),
                boost::bind(&Server::handle_accept, this, new_connection,
                    boost::asio::placeholders::error));
        return; 
    }
    void handle_accept(Connection::pointer new_connection,
            const boost::system::error_code& error) {
        if (! error) {
            try {
                new_connection->start();
            }
            catch (Excpt::ConnectionException const& e) {
                logerror(e.what());
            }
        }
        else {
            new_connection->close();
        }

        start_accept();
    }
private:
    // socket io 用的 io_service
    static io_service& get_io_service() {
        static io_service ios;
        return ios;
    }
private:
    // 用于接收来自客户端的 tcp 网络连接 
    tcp::acceptor m_acceptor;
    std::thread   m_event_thread;
    std::size_t   m_acceptor_io_thread_n;
}; // class Engine::Server::Server

} // namespace Engine::Server
} // namespace Engine


#endif // E_SERVER_H_1

