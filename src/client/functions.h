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

// 修改 heartbeat 探测发送的默认时间间隔
// heartbeat 只有在网络 interval 秒内，既没有读也没有写的情况下启动
void ConnSetPingInterval(uint32_t interval) {
    Engine::Client::Connection::Instance().set_ping_interval(interval);
}

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
// 断线回调会先于此回调执行
void ConnRegisterReadErrorCallback(CallbackConstCharPtrInt callback) {
    Engine::Client::Connection::Instance().register_read_error_callback(
            std::bind(callback, std::placeholders::_1, std::placeholders::_2));
}

// 只要连接可用，数据就一定会被写完， write_error_callback 被调用意味着:
// 1. 连接已经不可用
// 2. 给出出错信息、错误码, 和未写入的数据
// 3. 注意，断线回调 先于 write_error_callback 执行
void ConnRegisterWriteErrorCallback(
        CallbackConstCharPtrIntConstBytePtrSize callback) {
    Engine::Client::Connection::Instance().register_write_error_callback(
            std::bind(callback, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4)
    );
}

// send message
// 如果 data == nullptr, 则函数返回 false 
// callback(bytes_transferred), 
//
// bytes_transferred 实际传输的字节数，
//    包含 Engine 本身的包头长度 ConnPacketHeaderSize()
bool ConnSendMessage(const char* data, std::size_t data_len, 
        CallbackSize callback) {
    if (data == nullptr)  return false;
    Engine::Client::Connection::Instance().send_message(
            { 
              (Engine::const_byte_ptr)data, 
              (Engine::const_byte_ptr)(data + data_len)
            }, 
            callback);
    return true;
}

size_t ConnPacketHeaderSize(void) {
    return Engine::packet::header_size;    
}


// uuid
bool SetUUID(const char* uuid) {
    if (nullptr == uuid) return false;
    try {
        using Connection = Engine::Client::Connection;
        using uuid_t     = Engine::uuid_t;
        uuid_t u(uuid);
        return Connection::Instance().m_uuid.set(std::move(u));
    }
    catch (std::runtime_error const&) {
        return false; 
    }
}

const char* GenUUID(void) {
    static Engine::uuid_t u;
    return u.to_string().c_str();
}

} // extern "C"
#endif // EC_FUNCTIONS_H_1
