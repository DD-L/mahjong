#ifndef E_CONFIG_SERVER_H_1
#define E_CONFIG_SERVER_H_1
/*************************************************************************
	> File Name:    src/engine/config_server.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/2 10:13:19
 ************************************************************************/
#include <engine/config.h>

namespace Engine {
// namespace Engine::Server
namespace Server {

class Config final : public Engine::Config {
private:
    Config(void) = default;
    Config(const Config&) = delete;
    Config& operator= (const Config&) = delete;
public:
    static Config& get_instance(void) {
        static Config instance;
        return instance;
    }
    virtual void configure(const sdata_t& config_file) final;
    const sdata_t& get_bind_addr(void) const {
        return m_bind_addr;
    }
    uint16_t get_bind_port(void) const {
        return m_bind_port;
    }
    bool get_zip_on(void) const {
        return m_zip_on;
    }
    uint16_t get_timeout(void) const {
        return m_timeout;
    }
    const sdata_t& get_errfile(void) const {
        return m_errfile;
    }
    /*
     * 来自基类
    virtual uint16_t get_ping_interval(void) const override;
    */
    std::size_t get_event_io_thread(void) const {
        return m_event_io_thread;
    }
    std::size_t get_acceptor_io_thread(void) const {
        return m_acceptor_io_thread;
    }
    uint16_t get_max_heartbeat_lose(void) const {
        return m_max_heartbeat_lose;
    }
    uint16_t get_reconnection_delay(void) const {
        return m_reconnection_delay;
    }


private:
    sdata_t     m_bind_addr;
    sdata_t     m_errfile;            // = "";
    std::size_t m_event_io_thread;    // = 4;
    std::size_t m_acceptor_io_thread; // = 2;
    uint16_t    m_timeout;            // = 90;
    uint16_t    m_bind_port;
    uint16_t    m_max_heartbeat_lose; // 2
    uint16_t    m_reconnection_delay; // 25
    bool        m_zip_on;             // = false;
}; // class Engine::Server::Config

} // namespace Engine::Server
} // namespace Engine
#endif // E_CONFIG_SERVER_H_1
