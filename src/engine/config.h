#ifndef E_CONFIG_H_1
#define E_CONFIG_H_1
/*************************************************************************
	> File Name:    src/engine/config.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/2 10:09:15
 ************************************************************************/

#include <engine/typedefine.h>

namespace Engine {

class Config {
    virtual void configure(const sdata_t& config_file) = 0;
public:
    virtual ~Config(void) {}
    virtual uint16_t get_ping_interval(void) const {
        return 120; // seconds
    }
}; // class Engine::Config

} // namespace Engine

#endif // E_CONFIG_H_1
