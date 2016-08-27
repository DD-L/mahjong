#ifndef EC_CONFIG_H_1
#define EC_CONFIG_H_1
/*************************************************************************
	> File Name:    src/client/config.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/5 9:55:48
 ************************************************************************/

#include <engine/config.h>

namespace Engine {
namespace Client {

// Engine::Client::Config
class Config final : public Engine::Config {
    // TODO
    virtual void configure(const sdata_t& config_file) final {
        (void)config_file; 
        // reset m_ping_interval
        //m_ping_interval = 120;
    }
public:
    static Config& get_instance(void) {
        static Config instance;
        return instance;
    }
    virtual uint16_t get_ping_interval(void) const override {
        return m_ping_interval;
    }
private:
    Config(void)
        : m_ping_interval(Engine::Config::get_ping_interval()) {}
    Config(const Config&) = delete;
    Config& operator= (const Config&) = delete;
private:
    uint16_t m_ping_interval;
}; // class Engine::Clinet::Config

} // namespace Engine::Client
} // namespace Engine

#endif // EC_CONFIG_H_1
