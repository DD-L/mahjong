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

int main(int argc, char* argv[]) {
    
    _print_s("[INFO] start log output thread...\n");
    // 启动日志输出线程
    std::thread thread_logoutput(
            std::bind(&Engine::Log::output_thread,
            // TODO
            ""
            //Engine::Server::config::get_instance().get_logfilename()
            ));
    thread_logoutput.detach();
    try {

        EServer<Mahjong::Connection> s;
        Event::get_instance().post(test, 1);
        Event::get_instance().post(test, 2);
        Event::get_instance().post(test, 3);
        Event::get_instance().post(test, 4);
        Event::get_instance().post(test, 5);
        Event::get_instance().post(test, 6);

        std::thread t(ting, std::ref(s));
        t.detach();
        s.run("127.0.0.1", 2222, 2);
        while (! s.stopped())
            std::this_thread::sleep_for(std::chrono::seconds(1));

    }
    catch (boost::system::system_error const& e) {
        std::cout << e.what() << endl;
        std::cout << e.code() << endl;
    }
    loginfo("Main thread exit.");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
