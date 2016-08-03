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
    explicit Connection(boost::asio::io_service& io_service)
        : BASETYPE(io_service) {}
private:
    //virtual void start(void) throw(Engine::ConnectionException) override;
}; // Mahjong::Connection

} // namespace Majong

#endif // MJ_CONNECTION_H_1
