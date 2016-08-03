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
    m_timeout   = pt.get<uint32_t>("EngineServer.timeout", 90);
    m_errfile   = pt.get<sdata_t>("EngineServer.errfile", "");
    m_ping_interval = pt.get<uint32_t>("EngineServer.ping_interval", 120);
    m_event_io_thread
        = pt.get<std::size_t>("EngineServer.event_io_thread", 4);
    m_acceptor_io_thread 
        = pt.get<std::size_t>("EngineServer.acceptor_io_thread", 2);

    // print
    _print_s("[INFO] bind_addr:          " << m_bind_addr << std::endl);
    _print_s("[INFO] bind_port:          " << m_bind_port << std::endl);
    _print_s("[INFO] zip_on:             " 
            << std::boolalpha << m_zip_on << std::endl);
    _print_s("[INFO] timeout:            " << m_timeout << std::endl);
    _print_s("[INFO] errfile:            " << m_errfile << std::endl);
    _print_s("[INFO] ping_interval:      " << m_ping_interval << std::endl);
    _print_s("[INFO] event_io_thread:    "<< m_event_io_thread << std::endl);
    _print_s("[INFO] acceptor_io_thread: "<< m_acceptor_io_thread << std::endl);
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
