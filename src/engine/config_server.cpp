/*************************************************************************
	> File Name:    src\engine\config_server.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/2 10:20:11
 ************************************************************************/

#include <engine/config_server.h>
#include <engine/log.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

void Engine::Server::Config::configure(const sdata_t& config_file) 
try {
    ptree pt;
    read_json(config_file, pt);

    m_bind_addr = pt.get<sdata_t>("EngineServer.bind_addr");
    m_bind_port = pt.get<uint16_t>("EngineServer.bind_port");
    m_zip_on    = pt.get<bool>("EngineServer.zip_on", false);
    m_timeout   = pt.get<uint16_t>("EngineServer.timeout", 40);
    m_errfile   = pt.get<sdata_t>("EngineServer.errfile", "");
    m_event_io_thread
        = pt.get<std::size_t>("EngineServer.event_io_thread", 4);
    m_acceptor_io_thread 
        = pt.get<std::size_t>("EngineServer.acceptor_io_thread", 2);
    m_max_heartbeat_lose
        = pt.get<std::uint16_t>("EngineServer.max_heartbeat_lose", 2);
    m_reconnection_delay
        = pt.get<std::uint16_t>("EngineServer.reconnection_delay", 25);

    // print
    using std::endl;
    _print_s("[INFO] bind_addr:          " << m_bind_addr << endl);
    _print_s("[INFO] bind_port:          " << m_bind_port << endl);
    _print_s("[INFO] zip_on:             " 
            << std::boolalpha << m_zip_on << endl);
    _print_s("[INFO] timeout:            " << m_timeout << endl);
    _print_s("[INFO] max_heartbeat_lose: " << m_max_heartbeat_lose << endl);
    _print_s("[INFO] errfile:            " << m_errfile << endl);
    _print_s("[INFO] reconnection_delay: " << m_reconnection_delay << endl);
    _print_s("[INFO] event_io_thread:    " << m_event_io_thread << endl);
    _print_s("[INFO] acceptor_io_thread: " << m_acceptor_io_thread << endl);
}
catch (const ptree_error& e) {
    _print_s_err("[FATAL] " << e.what() 
            << ". An error occurred when reading the configuration file '" 
            << config_file << "'" << std::endl);
    exit(2);
}
catch (...) {
    _print_s_err("[FATAL] An error occurred when reading the configuration "
            "file '" << config_file << "'" << std::endl);
    exit(2);
}
