/*************************************************************************
	> File Name:    test.cpp
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/22 11:18:56
 ************************************************************************/

#include <iostream>
#include <client/functions.h>
using namespace std;


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
        cout << endl;
    }
}

void debug_message_callback(const char* message) {
    cout << message << endl;
}

void send_message_callback(int error_code, const char* error_message,
        const char* message, size_t message_len, size_t bytes_transferred) {
    if (error_code) {
        // error
        //return;
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

    cout << "error_code = " << error_code << endl;
    cout << "error_message = " << error_message << endl;
    // 要发送的 数据
    cout << "message = '";
    if (error_code == -1) {
        cout << "null";
        assert(message == nullptr);
    }
    else {
        for (size_t i = 0; i < message_len; ++i) {
            cout << *(message + i);
        }
    }
    cout << "'" << endl;
    cout << "bytes_transferred = " << bytes_transferred << endl;
}

int main() {

    // test Debug
    ConnRegisterDebugMessageCallback(&debug_message_callback);
    
    // test ConnRun
    ConnRun(&run_callback);
    ConnRun(&run_callback);

    // test ConnStart
    ConnStart("127.0.0.1", "2222", &start_callback);
    ConnStart("127.0.0.1", "2222", &start_callback);

    ConnRegisterReceivedCallback(&received_callback);
    ConnRegisterReceivedCallback(&received_callback);

    // 下面语句应该放到 start_callback(0) 中去。
    std::this_thread::sleep_for(std::chrono::seconds(4)); // temp
    // wait for the connection is ready.
    ConnSendMessage("hello", 5, &send_message_callback);
    ConnSendMessage("world", 5, &send_message_callback);
    ConnSendMessage(nullptr, 0, &send_message_callback);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}

