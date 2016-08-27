/*************************************************************************
	> File Name:    test.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/22 11:18:56
 ************************************************************************/

#include <iostream>
#include <client/functions.h>
using namespace std;

void disconnect_callback(void) {
    cout << "Connection is unavailable" << endl;
}

void read_error_callback(const char* msg, int code) {
    cout << "read_error: " << msg << ", code = " << code << endl;
}

void write_error_callback(const char* msg, int code,
        const uint8_t* rest, size_t size) {
    cout << "write_error: " << msg << ", code = " << code;
    if (size > 0) {
        cout << ". unfinished data: [" << size << "] bytes: ";
        while (size--)
        for (size_t i = 0; i < size; ++i)
            cout << hex << "0x" << int(*(rest + i)) << " "; 
    }
    cout << dec << endl;
}
    // switch (error_code) {
    // case 0:
    //  // 有数据被发送出去， 发送字节 为 bytes_transferred
    // break;
    // case -1:
    //  // message == nil
    // break;
    // case 2:
    //  // boost::asio::error::eof  与远程 socket 4次握手 close
    // break;
    // case 10054:
    // break;
    //  // boost::asio::error::connection_reset 对方直接关闭程序. tcp 协议栈发出 RESET 消息
    // case 995:
    //  // boost::asio::error::operation_aborted
    // break;
    // case 10009:
    // case 9:
    //  // bad_file_descriptor
    //  // socket 已经被关闭
    // break;
    // }

void run_callback(const char* msg) {
    cout << msg << endl;
}

void start_callback(int code) {
    switch (code) {
    case -1:
        cout << "socket already opening." << endl;
        break;
    case 0: // !!!!
        cout << "the connection is successful." << endl;
        break;
    case 1:
        cout << "host not found." << endl;
        break;
    case 2:
        cout << "network unavailable." << endl;
        break;
    default:
        cout << "unkown startd code = " << code << endl;
        break;
    }
}

void received_callback(const char* data, size_t length) {
    if (data) {
        cout << "Received Data: ";
        for (size_t i = 0; i < length; ++i) {
            cout << hex << (int)*(data + i) << ' '; 
        }
        cout << dec << endl;
    }
}

void debug_message_callback(const char* message) {
    cout << message << endl;
}

void send_message_callback(size_t bytes_transferred) {
    cout << "send_message_callback: " 
        << "bytes_transferred = " 
        << bytes_transferred << endl;
}

int main() {

    // get/set uuid
    //assert(SetUUID(GenUUID()));
    assert(SetUUID("{b67d855c-43bc-4a10-bad4-7378c0156fac}"));

    // 重置 heartbeat 周期，默认 120s
    //ConnSetPingInterval(5);

    // 注册 Debug Message
    ConnRegisterDebugMessageCallback(&debug_message_callback);
    // 注册断线回调
    ConnRegisterDisconnectCallback(&disconnect_callback);
    // 注册 read 出错回调
    // 断线回调会先于此回调执行
    ConnRegisterReadErrorCallback(&read_error_callback);
    // 注册 write 出错回调
    // 断线回调会先于此回调执行
    ConnRegisterWriteErrorCallback(&write_error_callback);


    // 注册read回调
    ConnRegisterReceivedCallback(&received_callback);
    // 测试重复注册
    ConnRegisterReceivedCallback(&received_callback);

    // test ConnRun
    ConnRun(&run_callback);
    ConnRun(&run_callback);

    // test ConnStart
    ConnStart("127.0.0.1", "2222", &start_callback);
    ConnStart("127.0.0.1", "2222", &start_callback);


    // 下面语句应该放到 start_callback(0) 中去。
    std::this_thread::sleep_for(std::chrono::seconds(4)); // temp
    // wait for the connection is ready.
    ConnSendMessage("hello", 5, &send_message_callback);
    ConnSendMessage("world", 5, &send_message_callback);
    ConnSendMessage(nullptr, 0, &send_message_callback);

    std::this_thread::sleep_for(std::chrono::seconds(2000));
    return 0;
}

