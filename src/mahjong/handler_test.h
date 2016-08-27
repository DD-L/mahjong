#ifndef HANDLER_TEST_H_1
#define HANDLER_TEST_H_1
/*************************************************************************
	> File Name:    src/mahjong/handler_test.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/26 9:49:27
 ************************************************************************/


// 本文件中的代码，只是演示用的
// 使用责任链模式，来处理 客户端发来的数据

#include <engine/log.h>
#include <engine/event.h>

namespace Mahjong {


class handler_test;
typedef std::shared_ptr<handler_test> shared_handler_test_t;

typedef std::weak_ptr<Engine::Server::Session> weak_session_t;
typedef std::shared_ptr<Engine::Server::Session> shared_session_t;
typedef std::shared_ptr<Engine::Server::Connection> shared_connection_t;

// handler 抽象类
class handler_test : public std::enable_shared_from_this<handler_test> {
    weak_session_t            m_session; // 不可使用 shared_ptr, 否则无法析构它
protected:
    shared_handler_test_t     m_successor = nullptr;
    Engine::Event&            m_event;
    // 非必须的; 专供 Event::strand_post 使用.
    // 如果想让 Event 按照 特定的(post 的先后) 顺序处理异步事件, 
    // 那么就使用  Event::strand_post, 否则使用 Event::post 即可
    Engine::io_service::strand&  m_strand;
public:
    handler_test(const weak_session_t& session, 
            Engine::io_service::strand& strand)
        : m_session(session),
          m_event(Engine::Event::get_instance()),
          m_strand(strand) {}
    void set_successor(shared_handler_test_t successor) {
        m_successor = successor;
    }
    virtual void handle_request(Engine::shared_data_type) = 0;
protected:
    template <typename DataType>
    void send_message(DataType&& data) {
        if(shared_session_t session = m_session.lock()) {
            if (shared_connection_t conn
                    = session->get_connection_weak_ptr().lock()) {
                conn->send_message(std::forward<DataType>(data));
            }
            else {
                logwarn("Connection has been lost."); 
            }
        }
    }
    template <typename DataType, typename SendSucceededHandler>
    void send_message(
            DataType&& data, 
            SendSucceededHandler&& succeeded_handler) {
        if(shared_session_t session = m_session.lock()) {
            if (shared_connection_t conn
                    = session->get_connection_weak_ptr().lock()) {
                conn->send_message(
                        std::forward<DataType>(data),
                        std::forward<SendSucceededHandler>(succeeded_handler));
            }
            else {
                logwarn("Connection has been lost."); 
            }
        }
    }
}; // class Mahjong::handler_test

#include <mahjong/connection.h> // for Engine::Server::Connection 
// handler 具体类
class handler_test1 : public handler_test {
public:
    using handler_test::handler_test; // 继承基类的构造函数
    virtual void handle_request(Engine::shared_data_type shared_data) override {
        // 处理 "hello"
        if (*shared_data == Engine::const_byte_ptr("hello")) {
            // 放到 Event 中处理 
            m_event.post(
                    std::bind(&handler_test1::handler, this, 
                        shared_from_this()));
        }
        else if (m_successor) {
            // 交给下任者
            m_successor->handle_request(shared_data);
        }
    }
private:
    // 真正的事件处理函数（在 event 中被执行）
    void handler(shared_handler_test_t) {
        loginfo("********* handler_test1: hello **********");

        loginfo("send 'say hello from server.' to client");
        // 反馈给客户端
        Engine::data_t data(
                Engine::const_byte_ptr("say hello from server."));
        send_message(std::move(data));

    }
}; // class Nahjong::handler_test1

class handler_test2 : public handler_test {
public:
    using handler_test::handler_test; // 继承基类的构造函数
    virtual void handle_request(Engine::shared_data_type shared_data) override {
        // 处理 world
        if (*shared_data == Engine::const_byte_ptr("world")) {
            // 放到 Event 中处理, 这次使用 strand 
            // 使所有通过 strand 异步执行的 事件，都有序进行
            m_event.strand_post(m_strand,
                    std::bind(&handler_test2::handler, this,
                        shared_from_this()));
        }
        else if (m_successor) {
            // 交给下任者
            m_successor->handle_request(shared_data);
        }
    }
private:
    // 真正的事件处理函数（在 event 被执行）
    void handler(shared_handler_test_t) {
        loginfo("********* handler_test2: world *********");
    }
}; // class Nahjong::handler_test2

// 边界
class handler_test_boundary : public handler_test {
public:
    using handler_test::handler_test; // 继承基类的构造函数
    virtual void handle_request(Engine::shared_data_type shared_data) override {
        // 处理边界
        //if (*shared_data == Engine::const_byte_ptr("hello")) {
            // 放到 Event 中处理 
            m_event.post(
                    std::bind(&handler_test_boundary::handler, this,
                        shared_from_this()));
        // 这里是边界, 没有下任者
        //}
        //else if (m_successor) {
        //    // 交给下任者
        //    m_successor->handle_request(shared_data);
        //}
    }
private:
    // 真正的事件处理函数（在 event 中被执行）
    void handler(shared_handler_test_t) {
        loginfo("********* handler_test_boundary: unknown data **********");

        loginfo("send 'say unknown data from server' to client");
        // 反馈给客户端
        Engine::data_t data(
             Engine::const_byte_ptr("say unknown data from server."));
        send_message(std::move(data));
    }
}; // class Nahjong::handler_test_boundary


} // namespace Mahjong

#endif // HANDLER_TEST_H_1
