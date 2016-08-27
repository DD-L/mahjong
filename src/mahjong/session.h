#ifndef M_SESSION_H_1
#define M_SESSION_H_1
/*************************************************************************
	> File Name:    src/mahjong/session.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/25 6:52:57
 ************************************************************************/
#include <engine/typedefine.h>
#include <engine/session.h>

// 使用责任链模式来 演示 session 的处理
#include <mahjong/handler_test.h>

namespace Mahjong {

using shared_data_type = Engine::shared_data_type;    

class Session : public Engine::Server::Session {
    // 责任链
    std::atomic_flag m_chain_of_responsibility_flag = ATOMIC_FLAG_INIT;
    std::vector<shared_handler_test_t> m_chain_of_responsibility;
    void initial_chain(void);
public:
    using shared_connection_t = Engine::Server::Session::shared_connection_t;
    using shared_uuid_t       = Engine::Server::Session::shared_uuid_t;
    Session(void); //= default;
    virtual ~Session(void);

//////////////////////////////////////
// Functions inherit from Engine::Server::Session
//////////////////////////////////////
private:
    virtual void data_process(shared_data_type shared_data) override;
//public:
    //// 设备掉线处理
    //void dropped(void);
    //// 更新持有的 connection
    //void update(shared_connection_t conn);
    //// 更新持有的 uuid
    //void update(shared_uuid_t uuid);
    //// 同时更新持有的 uuid 和 connection
    //void update(shared_connection_t conn, shared_uuid_t uuid);
    //void update(shared_uuid_t uuid, shared_connection_t conn);
    //// 获取持有的 uuid 共享指针
    //shared_uuid_t uuid(void);
    //// 获取持有的 uuid 弱共享指针
    //std::weak_ptr<uuid_t> get_uuid_weak_ptr(void);
    //// 获取持有的 connection 弱共享指针, 传递时不会改变 connection 的引用计数
    //std::weak_ptr<Engine::Server::Connection> get_connection_weak_ptr(void);
protected:
    //////////////////////////////////////
    // Variables inherit from Engine::Server::Session
    //////////////////////////////////////
    //shared_uuid_t        m_uuid;

private:
    // 可以被用来序列化异步操作
    Engine::io_service::strand m_strand;
}; // class Mahjong::Session

//
// 对于 Mahjong::Session 而言, 成员变量 *m_connection
// 可以使用一个 send_message 公有成员方法, 用
// 来向 client 发送数据. 具有下面几个可以访问的重载形式:
//
// (1) template <typename DataType> void send_message(DataType&& data);
// (2) template <typename DataType, typename SendSucceededHandler>
//     void send_message (
//            DataType&& data,
//            SendSucceededHandler&& succeeded_handler // 指定发送成功后的回调
//     );
//
// data 必须是 Engine::data_t 类型, 推荐传递右值的方式, 来向客户端发送消息
//

}; // namespace Mahjong



#endif // M_SESSION_H_1
