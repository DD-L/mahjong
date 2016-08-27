/*************************************************************************
	> File Name:    src/engine/typedefine.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/14 11:51:44
 ************************************************************************/

#ifndef E_TYPEDEFINE_H_1
#define E_TYPEDEFINE_H_1

#include <except/except.h>
#include <engine/macros.h>

#include <stdexcept>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <type_traits>
#include <system_error>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Engine {

// type
typedef uint8_t                 byte;
typedef uint16_t                data_len_t;
typedef uint16_t                keysize_t;

typedef byte*                   byte_ptr;
typedef const byte*             const_byte_ptr;

// Engine::data_t
typedef std::basic_string<byte> data_t;    
typedef std::string             sdata_t;
typedef std::vector<byte>       vdata_t;

// Engine::shared_data_type
typedef std::shared_ptr<data_t> shared_data_type;


////////////////////////////////////////////////////////////////////////////
// 
// std::mutex 开关 
//
// Engine::UseStdMutex<bool>
//
////////////////////////////////////////////////////////////////////////////
// 有
// sizeof (UseStdMutex<true>) == sizeof (std::mutex) // == 40 (64bit)
// sizeof (UseStdMutex<false>) == 1
//
// std::is_same<UseStdMutex<true>, std::mutex>::value == true 
// std::is_same<UseStdMutex<flase>, std::mutex>::value == false 
//

// namespace Engine::Details
namespace Details {

template<bool>
struct __usestdmutex {
    typedef std::mutex Type;
};

template<>
struct __usestdmutex<false> {
private:
    struct __empty {
        // 保持与 std::mutex 的 成员函数名 和 成员函数签名 完全一致
        constexpr __empty() noexcept = default;
        __empty(const __empty&) = delete;
        __empty& operator= (const __empty&) = delete;
        ~__empty() = default;
        
        void lock(void) {}
        bool try_lock(void) { return true; }
        void unlock(void) {}
        std::mutex::native_handle_type native_handle(void) {
            if (std::is_pointer<std::mutex::native_handle_type>::value)
                return nullptr;
            return std::mutex::native_handle_type();
        }
    };
public:
    typedef __empty Type;
};
} // namespace Engine::Details

// class Engine::UseStdMutex<bool>
template<bool value>
using UseStdMutex = typename Details::__usestdmutex<value>::Type;


/////////////////////////////////////////////////////////
// class Engine::test_and_set_value  thread-safe
/////////////////////////////////////////////////////////
// 线程安全的设置定一个值，保证它只能被初始化一次，或设定 set 一次。
//
// 1. 获取该值时，若此前 未曾初始化 同时也未被设定 set，则会抛出
// 一个 std::runtime_error 的异常。
// 2. 被初始化后，仍然可以被设定 set 一次。
// 3. 设定成功, set(value) 返回 true
// 4. 尝试多次设定，则 set(value) 返回 false
//
// 示例:
//
// test_and_set_value<int> i(100);
// cout << i << endl;                         // 100
// cout << i.value() << endl;                 // 100;
// assert(i.set(200) == true);
// cout << i.value() << endl;                 // 200;
// assert(i.set(200) == false);
//
//
// test_and_set_value<string> s;
// try {
//     s.value(); // throw std::runtime_error
// }
// catch(std::exception const& e) { 
//    cout << e.what() << endl; 
//    // "test_and_set_value::value(): An uninitialized object or not set"
// }
// assert(s.set("hello") == true);
// cout << s.value() << endl;                 // hello
// cout << (string)s << endl;                 // hello
// assert(s.set("world") == false);
//
// struct C {
//     C() = delete;
//     C(int) {}
// };
// test_and_set_value<C> c;
// assert(c.set(C(100)) == true);
// C cc(200);
// test_and_set_value<C> c2(cc);
// assert(c2.set(C(500)) == true);
// assert(c2.set(cc) == false);
// test_and_set_value<C> c3(C(1));
/////////////////////////////////////////////////////////
template <typename ValueType>
class test_and_set_value final {
    std::shared_ptr<ValueType>m_value_ptr;
    std::atomic_flag           m_flag;
public:
    test_and_set_value(void) : 
        m_flag(ATOMIC_FLAG_INIT) {}
    test_and_set_value(const ValueType& initial_value) : 
        m_value_ptr(std::make_shared<ValueType>(initial_value)),
        m_flag(ATOMIC_FLAG_INIT) {}
    test_and_set_value(ValueType&& initial_value) : 
        m_value_ptr(std::make_shared<ValueType>(std::move(initial_value))),
        m_flag(ATOMIC_FLAG_INIT) {}

    test_and_set_value(const test_and_set_value&) = delete;
    test_and_set_value& operator= (const test_and_set_value&) = delete;
    ~test_and_set_value() = default;

    bool set(const ValueType& value) {
        return _set([this, &value]() {
            m_value_ptr = std::make_shared<ValueType>(value);
        });
    }
    bool set(ValueType&& value) {
        return _set([this, &value]() {
            m_value_ptr = std::make_shared<ValueType>(std::move(value));
        });
    }

    const ValueType& value(void) const throw (std::runtime_error) {
        if (m_value_ptr)
            return *m_value_ptr;
        throw std::runtime_error(
             "test_and_set_value::value(): An uninitialized object or not set");
    }

    operator const ValueType&(void) const throw (std::runtime_error) {
        return value();
    }
private:
    template<typename Func>
    bool _set(Func f) {
        if (! m_flag.test_and_set()) {
            f();
            return true;
        }
        return false;
    }
}; // class Engine::test_and_set_value

///////////////////////////////////////////////////
// boost
///////////////////////////////////////////////////

//namespace Engine::asio;
namespace asio = boost::asio;
using namespace asio;
using ip::tcp;
using ip::udp;


///////////////////////////////////////////////////
// functions
///////////////////////////////////////////////////

/**
 * @brief make_shared_data 
 * @param data_length
 * @param v
 * @return              返回一个容器长度为 data_length, 且被 v 
 *                      填充的 shared_data_type 类型临时对象 
 */
static inline shared_data_type
make_shared_data(const std::size_t data_length,
        data_t::value_type v) {
    return std::make_shared<data_t>(data_length, v);
}
/**
 * @brief make_shared_data
 * @param data 默认值为 data_t()
 * @return     返回一个shared_data_type 类型临时对象, 此对象指向一个 data 的副本
 */
static inline shared_data_type
make_shared_data(const data_t& data) {
    return std::make_shared<data_t>(data);
}
static inline shared_data_type
make_shared_data(data_t&& data) {
    return std::make_shared<data_t>(std::move(data));
}
static inline shared_data_type 
make_shared_data(void) {
    return std::make_shared<data_t>();
}

// make_shated_data(std::begin, std::end);
template <typename Iterator>
static inline shared_data_type
make_shared_data(const Iterator& begin, const Iterator& end) {
    return make_shared_data({begin, end});
}

// make_shared_data(data_t/vdata_t/sdata_t)
template <typename DATATYPE>
static inline shared_data_type
make_shared_data(DATATYPE&& data) {
    return make_shared_data(
            std::forward<DATATYPE>(data).begin(), 
            std::forward<DATATYPE>(data).end()
    );
}


// 将 asio::mutable_buffer 或 asio::const_buffer 转换成 shared_data_type
// 或将 
//      boost::array<asio::mutable_buffer, N> 
//   或 boost::array<asio::const_buffer, N>
// 转换成 shared_data_type
template <typename BufferSequence>
static inline shared_data_type 
buffer2shared_data(const BufferSequence& __buffer) {
    //vdata_t _data(boost::asio::buffer_size(buffer));
    //boost::asio::buffer_copy(boost::asio::buffer(_data), buffer);
    //return make_shared_data(std::move(_data));
    auto&& buffer = asio::buffer(__buffer);
    auto begin(asio::buffer_cast<byte_ptr>(buffer)); // const_byte_ptr ?? TODO
    const size_t size = asio::buffer_size(buffer);
    return make_shared_data({begin, begin + size});
}

/**
 * @brief _debug_format_data 格式化一组数据
 * @param data        要打印的容器
 * //@param data_length 能打印容器的最大长度
 * @param VTYPE       容器元素打印的强制转换类型
 * @param c           容器元素之间输出间隔符; 默认值char(' '), c = 0 时无间隔符
 * @param f           流输出控制函数; 默认值 std::dec
 * @return 返回一个 sdata_t 的临时对象
 */
template <typename DATA_TYPE, typename VTYPE, typename F = decltype(std::dec)>
sdata_t _format_data(const DATA_TYPE& data, VTYPE, 
        char c = ' ', F f = std::dec) {
    std::ostringstream oss;
    //oss.flush();
    for (auto& v : data) {
        oss << f << VTYPE(v) << c;
    }
    return oss.str();
}
template <typename DATA_TYPE, typename VTYPE, typename F = decltype(std::dec)>
//std::ostringstream& _debug_format_data(const DATA_TYPE& data, VTYPE, 
sdata_t _debug_format_data(const DATA_TYPE& data, VTYPE, 
        char c = ' ', F f = std::dec) {
#ifdef ENGINE_DEBUG
    return std::move(_format_data(data, VTYPE(), c, f));
#endif
    return "";
}


/**
 * @brief get_vdata_from_packet: 将 'packet' 里的数据 转换为 vdata_t 类型 
 * @param packet:
 *      'packet' 可以是下列类型的对象:
 *      Engine::Client::reply, Engine::Client::request, 
 *      Engine::Server::reply, lproxy::Server::request
 * @return 返回一个 vdata_t 类型的临时对象
 */
template <typename T>
vdata_t get_vdata_from_packet(T&& packet) {
    vdata_t _test(boost::asio::buffer_size(std::forward<T>(packet).buffers()));
    boost::asio::buffer_copy(boost::asio::buffer(_test), 
            std::forward<T>(packet).buffers());
    return _test;
}

///////////////////////////////////////////////////
// namespace Engine::placeholders
///////////////////////////////////////////////////
namespace placeholders {
// Engine::placeholders::shared_data
static auto&& shared_data = make_shared_data();
} // namespace Engine::placeholders

///////////////////////////////////////////////////
// Engine::readbuffer
///////////////////////////////////////////////////
namespace readbuffer {
enum { max_length = 4096, length_handshake = 1024 };
} // namespace Engine::readbuffer
// 等价于下面, 但失去了与 int 的隐式转换特性
//enum class readbuffer { max_length = 4096, length_handshake = 1024 };


///////////////////////////////////////////////////
// engine  Connection 的状态
///////////////////////////////////////////////////
namespace ConnetionStatus {
    enum status_t : byte {
        status_not_connected = 0x00,
        status_connected     = 0x01,
        status_hello         = 0x02,
        status_auth          = 0x03,
        status_iddata        = 0x04,
        status_data          = 0x05
    };
} // namespace Engine::Status

///////////////////////////////////////////////////
// exception
///////////////////////////////////////////////////

// namespace Engine::Excpt
namespace Excpt {

struct __Connection_Except {
    static std::string name() {
        return "Connection";
    }
};
// 连接时异常
typedef ExceptionTemplate<__Connection_Except> ConnectionException;

struct __Connection_wrong_packet_type {
    static std::string name() {
        return "wrong_packet_type";
    }
};
// packet 类型异常
typedef ExceptionTemplate<__Connection_wrong_packet_type> wrong_packet_type;

// packet 数据不完整 异常
class incomplete_data : public std::exception {
public:
    incomplete_data(int less) noexcept : less_(less) {}
    virtual ~incomplete_data(void) noexcept {}
    virtual const char* what(void) const noexcept {
        return "incomplete_data";
    }
    const int less(void) const {
        return less_;
    }
private:
    int   less_;
};

// engine Connection 状态异常
using status_t = ConnetionStatus::status_t;
class wrong_conn_status : public std::exception {
public:
    wrong_conn_status(status_t wrong_status, 
            status_t current_status) noexcept 
        : std::exception(), 
        m_wrong_status(wrong_status),
        m_current_status(current_status) {}
    virtual ~wrong_conn_status(void) noexcept {}
    virtual const char* what(void) const noexcept {
        static sdata_t map_status[6] = {
            "status_not_connected",
            "status_connected",
            "status_hello",
            "status_auth",
            "status_iddata",
            "status_data"
        };
        const_cast<wrong_conn_status*>(this)->m_message =
            "wrong_conn_status: '" + map_status[m_wrong_status]
            + "', current status = '" + map_status[m_current_status] + "'";
        return m_message.c_str() ;
    } 
private:
    status_t m_wrong_status;
    status_t m_current_status;
    sdata_t  m_message;
};

using EncryptException = ::EncryptException;
using DecryptException = ::DecryptException;
} // namespace Engine::Excpt
} // namespace Engine


// for Client (C lib)
typedef void (*CallbackVoid)(void);
typedef void (*CallbackInt)(int);
typedef void (*CallbackSize)(size_t);
typedef void (*CallbackIntSize)(int, size_t);
typedef void (*CallbackCharPtr)(char*);
typedef void (*CallbackCharPtrSize)(char*, size_t);
typedef void (*CallbackConstCharPtr)(const char*);
typedef void (*CallbackConstCharPtrSize)(const char*, size_t);
typedef void (*CallbackConstCharPtrInt)(const char*, int);
typedef void (*CallbackConstCharPtrIntConstBytePtrSize)(
        const char*, int, const uint8_t*, size_t);
typedef void (*CallbackIntConstCharPtrSize)(int, const char*, size_t);
typedef void (*CallbackIntConstCharPtrConstCharPtrSizeSize)(
        int, const char*, const char*, size_t, size_t);

#endif // E_TYPEDEFINE_H_1
