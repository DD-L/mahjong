#ifndef MJ_CONNECTION_H_1 
#define MJ_CONNECTION_H_1 
/*************************************************************************
	> File Name:    connection.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 10:26:49
 ************************************************************************/

#include <engine/connection.h>

namespace Mahjong {

class Connection 
    : public Engine::Server::Connection {
public:
    using shared_session_t = BASETYPE::shared_session_t;
    using Engine::Server::Connection::Connection;
private:
    // 必须重新该方法，使其返回自己定制的 Session
    virtual shared_session_t session_generator(void) override;
}; // Mahjong::Connection

} // namespace Majong

#endif // MJ_CONNECTION_H_1
