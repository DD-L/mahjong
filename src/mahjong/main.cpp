/*************************************************************************
	> File Name:    main.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/20 5:42:45
 ************************************************************************/


#include <engine/macros.h>
#include <engine/log.h>
#include <engine/server.h>

#include <mahjong/connection.h>
#include <program_options/program_options.h>

#include <iostream>
using namespace std;
using namespace Engine;


void test(int i) {
    loginfo("this is a test [" << i <<"]");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ting(EServer<Mahjong::Connection>& s) {
    Event::get_instance().post(test, 7);
    Event::get_instance().post(test, 8);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Event::get_instance().post(test, 9);
    s.stop();
}

////////////////////////////////////////////////////////////////////
static void process_program_options(const program_options& po,
        std::string& server_config_file) {
    if (po.empty()) {
        server_config_file = "server-config.json";
        return;
    }
    if (po.count("-h") || po.count("--help")) {
        std::string message = po.show_help("\"Server\" side of \"Engine\"");
        _print_s(message);
        exit(0);
    }

    if (po.count("-v") || po.count("--version")) {
        // TODO
        _print_s("build - Alpha" << std::endl);
        exit(0);
    }

    try {
        if (po.count("-c")) {
            std::string value = po.get("-c");
            server_config_file = value;
            return;
        }
        if (po.count("--config")) {
            std::string value = po.get("--config");
            server_config_file = value;
            return;
        }
    }
    catch (const program_options::parameter_error& error) {
        _print_s_err("[FATAL] " << error.what() << std::endl);
        exit(-1);
    }

    _print_s_err("[FATAL] Unsupported options" << std::endl);
    exit(1);
}

void option(int argc, char** argv, std::string& server_config_file) {
    assert(argv);
    // 参数处理
    program_options po("mahjong [option]");

    po.add_option("-h, --help", "Show this message.");
    po.add_option("-v, --version", "Show current version.");
    po.add_option("-c, --config <profile>",
            "Specify which configuration file 'mahjong' should\n"
            "use instead of the default.\n"
            "If not specified, the default configuration file is\n"
            "'server-config.json' in the current working directory.");

    po.example("mahjong");
    po.example("mahjong -c /path/to/server-config.json");

    po.store(argc, argv);
    process_program_options(po, server_config_file);
}

int main(int argc, char* argv[])
try {
    // get server_config_file
    std::string server_config_file;
    option(argc, argv, server_config_file);
    _print_s("[INFO] configuration file: " << server_config_file << std::endl);

    // 加载配置文件
    Engine::Server::Config::get_instance().configure(server_config_file);
    
    _print_s("[INFO] start log output thread...\n");
    // 启动日志输出线程
    std::thread thread_logoutput(
            std::bind(&Engine::Log::output_thread,
            Engine::Server::Config::get_instance().get_errfile()));
    thread_logoutput.detach();

    EServer<Mahjong::Connection> s;
    Event::get_instance().post(test, 1);
    Event::get_instance().post(test, 2);
    Event::get_instance().post(test, 3);
    Event::get_instance().post(test, 4);
    Event::get_instance().post(test, 5);
    Event::get_instance().post(test, 6);

    // 停止服务器
    //std::thread t(ting, std::ref(s));
    //t.detach();

    const sdata_t& bind_addr 
        = Engine::Server::Config::get_instance().get_bind_addr();
    uint16_t bind_port
        = Engine::Server::Config::get_instance().get_bind_port();
    s.run(bind_addr, bind_port);
    while (! s.stopped())
        std::this_thread::sleep_for(std::chrono::seconds(1));

    loginfo("Main thread exit.");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
catch (boost::system::system_error const& e) {
    _print_s_err("[FATAL] Exception: " << e.what() << ", code = " << e.code()
            << " " << Engine::Log::basename(__FILE__) << ":"
            << __LINE__ << std::endl);
    exit(1);
}
catch (const std::exception& e) {
    _print_s_err("[FATAL] Exception: " << e.what()
            << " " << Engine::Log::basename(__FILE__) << ":"
            << __LINE__ << std::endl);
    exit(1);
}
catch (...) {     
    _print_s_err("[FATAL] An error has occurred" 
            << " " << Engine::Log::basename(__FILE__) << ":" 
            << __LINE__ << std::endl);
    exit(1);
}
