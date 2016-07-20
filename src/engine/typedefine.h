/*************************************************************************
	> File Name:    typedefine.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/14 11:51:44
 ************************************************************************/

#ifndef E_TYPEDEFINE_H_1
#define E_TYPEDEFINE_H_1

#include <except/except.h>
#include <engine/macros.h>

#include <stdint.h>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <memory>
#include <type_traits>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

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

///////////////////////////////////////////////////
// boost
///////////////////////////////////////////////////
using namespace boost::asio;
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
static inline shared_data_type make_shared_data(const std::size_t data_length,
        data_t::value_type v) {
    return std::make_shared<data_t>(data_length, v);
}
/**
 * @brief make_shared_data
 * @param data 默认值为 data_t()
 * @return     返回一个shared_data_type 类型临时对象, 此对象指向一个 data 的副本
 */
template<typename DATATYPE>
static inline shared_data_type make_shared_data(DATATYPE&& data = data_t()) {
    return std::make_shared<DATATYPE>(std::forward<DATATYPE>(data));
}
static inline shared_data_type make_shared_data(void) {
    return std::make_shared<data_t>();
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
//std::ostringstream& _debug_format_data(const DATA_TYPE& data, VTYPE, 
sdata_t _debug_format_data(const DATA_TYPE& data, VTYPE, 
        char c = ' ', F f = std::dec) {
#ifdef ENGINE_DEBUG
    std::ostringstream oss;
    //oss.flush();
    for (auto& v : data) {
        oss << f << VTYPE(v) << c;
    }
    return oss.str();
#endif
    return "";
}

/**
 * @brief get_vdata_from_packet: 将 'lss_pack' 里的数据 转换为 vdata_t 类型 
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
// exception
///////////////////////////////////////////////////
struct __Connection_Except {
    static std::string name() {
        return "Connection";
    }
};
// 连接时异常
typedef ExceptionTemplate<__Connection_Except> ConnectionException;

} // namespace Engine

#endif // E_TYPEDEFINE_H_1
