/*************************************************************************
	> File Name:    src\mahjong\session.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/26 8:14:43
 ************************************************************************/

#include <mahjong/session.h>

#include <engine/event.h>
#include <engine/log.h>

namespace Mahjong {

Session::Session(void)
    : m_strand(Engine::Event::get_instance().get_io_service()) {
    elogdebug("Mahjong::Session::Session() "  << ". this=" << this);
}
Session::~Session(void) {
    elogdebug("Mahjong::Session::~Session(), " 
            << "uuid=" << *m_uuid << ". this=" << this);
}
void Session::initial_chain(void)
try {
    // 保证只会被正确的执行一次
    if (! m_chain_of_responsibility_flag.test_and_set()) {
        //  初始化责任链对象
        auto self(shared_from_this());
        // hello
        m_chain_of_responsibility.emplace_back(
                std::make_shared<handler_test1>(self, m_strand));
        // world
        m_chain_of_responsibility.emplace_back(
                std::make_shared<handler_test2>(self, m_strand));
        // ...
        //
        // anything else
        // 边界
        m_chain_of_responsibility.emplace_back(
                std::make_shared<handler_test_boundary>(self, m_strand));

        // 设置责任链的上下家关系
        for (auto&& it = m_chain_of_responsibility.begin() + 1;
                it != m_chain_of_responsibility.end(); ++it) {
            (*(it - 1))->set_successor(*it);
        }
    }
}
catch (...) {
    logerror(
    "An error occurred during initialization of the chain of responsibility, "
    "reset it...");
    // 如果设置责任链失败, 重置责任链
    m_chain_of_responsibility.clear();
    m_chain_of_responsibility_flag.clear();
}

void Session::data_process(shared_data_type shared_data) {
    loginfo(Engine::_format_data(*shared_data, char(), 0)
            << ". Mahjong::Session::data_process.");   

    // 大致执行步骤是：
    // step 1 解析 shared_data, 格式是跟客户端协商而来
    // step 2 将 解析后的 数据 放到 Event 中处理
    // step 3 在 Event 回调中, 执行 
    //          if (m_connection) {
    //              m_connection->tcp_socket_async_write(...);
    //          }


    // 使用样例中的责任链模式 代替 switch - case 结构 来处理:
    this->initial_chain();

    // 从第一个开始处理
    if (! m_chain_of_responsibility.empty())
        (*m_chain_of_responsibility.begin())->handle_request(shared_data);
    else
        logwarn("The chain of responsibility is not set");
}

} // namespace Mahjong


