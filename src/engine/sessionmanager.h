#ifndef E_SESSIONMANAGER_H_1
#define E_SESSIONMANAGER_H_1
/*************************************************************************
	> File Name:    sessionmanager.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 16:57:58
 ************************************************************************/
#include <engine/session.h>
namespace Engine {

// 线程安全的单件
class SessionMananger {
public:


    // test
    // TODO
    struct uuid{};
private:
    std::map<uuid*, std::shared_ptr<Session>> m_session_map;
}; // class SessionMananger

} // namespace Engine

#endif // E_SESSIONMANAGER_H_1
