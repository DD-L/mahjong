/*************************************************************************
	> File Name:    connection.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 10:46:23
 ************************************************************************/

#include <mahjong/connection.h>
#include <mahjong/session.h> // 自己定制的 Session

using namespace Engine;

namespace Mahjong {

Connection::shared_session_t Connection::session_generator(void) {
    // Mahjong::Session
    return std::make_shared<Session>();
}


} // namespace Mahjong
