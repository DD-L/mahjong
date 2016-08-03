#ifndef EC_FUNCTIONS_H_1
#define EC_FUNCTIONS_H_1
/*************************************************************************
	> File Name:    functions.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/22 14:14:25
 ************************************************************************/

#include <client/connection.h>
extern "C" {

// 启动连接服务
// callback(void) - 线程退出的时候执行的回调函数
void ConnRun(CallbackConstCharPtr callback) {
    Engine::Client::Connection::Instance().run(callback);
}

// 连接 server
// callback(int) - int 状态码，
// 0 连接成功，
// 非零 连接失败, -1 socket之前已经连接成功. 1 主机不可达， 2 网络不可达
void ConnStart(const char* server_name,
        const char* server_port,
        CallbackInt callback) {
    Engine::Client::Connection::Instance().start(
            server_name, 
            server_port,
            callback);
}

// 注册 receive message 后的回调函数
void ConnRegisterReceivedCallback(CallbackConstCharPtrSize callback) {
    Engine::Client::Connection::Instance().register_received_callback(
            std::bind(callback, std::placeholders::_1, std::placeholders::_2));
}

// 注册 debug message 回调
void ConnRegisterDebugMessageCallback(CallbackConstCharPtr callback) {
    Engine::Client::Connection::register_debug_message_callback(
            std::bind(callback, std::placeholders::_1));
}

// 注册 断线 后的回调函数。
// 断线回调会在 主动 close socket 后调用。 
void ConnRegisterDisconnectCallback(CallbackVoid callback) {
    Engine::Client::Connection::Instance().register_disconnect_callback(
            std::bind(callback));
}

// 注册 read error 后的回调函数，会给出失败原因及失败码
void ConnRegisterReadErrorCallback(CallbackConstCharPtrInt callback) {
    Engine::Client::Connection::Instance().register_read_error_callback(
            std::bind(callback, std::placeholders::_1, std::placeholders::_2));
}

// callback(error_code, ec.message, data, data_len, bytes_transferred), 
// ec == 0 成功, 
// ec != 0 失败, >0 时是错误码, < 0 时 data == nil
// bytes_transferred 实际传输的字节数，
//    包含 Engine 本身的包头长度 ConnPacketHeaderSize()
void ConnSendMessage(const char* data, std::size_t data_len, 
        CallbackIntConstCharPtrConstCharPtrSizeSize callback) {
    if (data) {
        Engine::Client::Connection::Instance().send_message(
                { data, data + data_len }, 
                callback);
    }
    else {
        callback(-1, "send_data == null", data, data_len, 0); // data == nullptr
    }
}

size_t ConnPacketHeaderSize(void) {
    return Engine::packet::header_size;    
}

//void SendMessage(const char* msg, msg_len, handler);
//void Regerster(handler);
}

#endif // EC_FUNCTIONS_H_1
