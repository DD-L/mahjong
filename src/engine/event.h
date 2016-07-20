/*************************************************************************
	> File Name:    event.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/15 11:32:08
 ************************************************************************/
#ifndef E_EVENT_H_1
#define E_EVENT_H_1

#include <engine/log.h>

namespace Engine {

class Event {
public:
    static Event& get_instance(void) {
        static Event event;
        return event;
    }

    // 阻塞
    void run(std::size_t thread_n = 1) {

        // 为了不使"ios没有任务, ios.run就立刻返回"
        boost::asio::io_service::work work(m_event_ios); 

        std::vector<std::thread> t; 
        while (thread_n--) {
            t.push_back(std::move(
                        std::thread(
                            std::bind(
                                &Event::thread_working, 
                                this//, 
                                //std::ref(...)
                            ))));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        for (auto& v : t) {
            v.join();
        }
        elogdebug("Event::run() exit");
    }

    void stop(void) {
        m_event_ios.stop();
    }

    bool stopped(void) const {
        return m_event_ios.stopped();
    }

    template<class Fn, class... Args>
    void post(Fn&& fn, Args&&... args) {
        m_event_ios.post(
                std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));
    }
    template<class Fn, class... Args>
    void strand_post(io_service::strand& strand, Fn&& fn, Args&&... args) {
        m_event_ios.post(
                strand.wrap(
                    std::bind(
                        std::forward<Fn>(fn), std::forward<Args>(args)...)));
    }

private:
    void thread_working(void) {
        for (;;) {
            try {
                m_event_ios.run();
                break;
            }
            catch (boost::system::system_error const& e) {
                logerror(e.what());
            }
            catch (const std::exception& e) {
                logerror(e.what());
            }
            catch (...) {
                logerror("An error has occurred. m_event_ios.run()");
            }
        }
        loginfo("one of event-threads exit!");
    }


private:
    // io_service 的个数 最佳应该 和 CPU 的核数相同
    // 除了 intel 的部分超线程处理器是 超线程 (线程数 = 核心数 * 2)
    // 其余的基本上都是 线程数 = 核心数
    //
    // unsigned concurentThreadsSupported = boost::thread::hardware_concurrency(); 可以获得 硬件线程数。
    // 
    // 如果有很多阻塞操作，一个 ios 可以跑在多个线程中去。
    //
    io_service m_event_ios;
    io_service m_socket_ios;
private:
    Event() = default;
    Event(const Event&) = delete;
    Event& operator= (const Event&) = delete;
    virtual ~Event() {}
}; // class Event

} // namespace Engine

#endif // E_EVENT_H_1
