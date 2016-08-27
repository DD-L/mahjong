#ifndef E_PACKET_H_1
#define E_PACKET_H_1
/*************************************************************************
	> File Name:    e_packet.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/7/18 17:29:58
 ************************************************************************/

#include <engine/typedefine.h>
namespace Engine {

struct __packet {
public:
    byte   version;
    byte   type;
    byte   data_len_high_byte,  data_len_low_byte;
    data_t data;
public:
    // 包头长度
    static constexpr size_t header_size(void) {
        return sizeof (byte) * 4;
        // version + type + data_len_high_byte + data_len_low_byte
    }
    __packet(void) : version(0xff), type(0xff), 
                       data_len_high_byte(0x00), data_len_low_byte(0x00),
                       data() {}
    
    __packet(byte version_,  byte pack_type_, 
           data_len_t data_len_, const data_t& data_)

            : version(version_), type(pack_type_), 
              data_len_high_byte((data_len_ >> 8) & 0xff),
              data_len_low_byte(data_len_ & 0xff), data(data_) {}

    __packet(byte version_, byte pack_type_,  byte data_len_high_, 
           byte data_len_low_, const data_t& data_)

            : version(version_), type(pack_type_), 
              data_len_high_byte(data_len_high_),
              data_len_low_byte(data_len_low_), data(data_) {}

    __packet(byte version_,  byte pack_type_, 
           data_len_t data_len_, data_t&& data_)

            : version(version_), type(pack_type_), 
              data_len_high_byte((data_len_ >> 8) & 0xff),
              data_len_low_byte(data_len_ & 0xff), 
              data(std::move(data_)) {}

    __packet(byte version_, byte pack_type_,  byte data_len_high_, 
           byte data_len_low_, data_t&& data_)

            : version(version_), type(pack_type_), 
              data_len_high_byte(data_len_high_),
              data_len_low_byte(data_len_low_), data(std::move(data_)) {}

    __packet(const __packet& that) {
        if (this != &that) {
            version = that.version;
            type    = that.type;
            data_len_high_byte = that.data_len_high_byte;
            data_len_low_byte  = that.data_len_low_byte;
            data.assign(that.data.begin(), that.data.end());
        }
    }
    __packet(__packet&& that) {
        if (this != &that) {
            version = that.version;
            type    = that.type;
            data_len_high_byte = that.data_len_high_byte;
            data_len_low_byte  = that.data_len_low_byte;
            data = std::move(that.data);
        }
    }
    __packet& operator= (const __packet& that) {
        if (this != &that) {
            version = that.version;
            type    = that.type;
            data_len_high_byte = that.data_len_high_byte;
            data_len_low_byte  = that.data_len_low_byte;
            data.assign(that.data.begin(), that.data.end());
        }
        return *this;
    }
    __packet& operator= (__packet&& that) {
        if (this != &that) {
            version = that.version;
            type    = that.type;
            data_len_high_byte = that.data_len_high_byte;
            data_len_low_byte  = that.data_len_low_byte;
            data = std::move(that.data);
        }
        return *this;
    }

    virtual ~__packet(void) {} 
}; // struct Engine::__packet

namespace packet {
enum  { header_size = __packet::header_size() };
}; // namespace Engine::packet

class request_and_reply_base_class {
public:
    virtual data_len_t data_len(void) const = 0;
    virtual ~request_and_reply_base_class() {}
};

enum class CommonType : byte {
    bad      = 0xff,
    hello    = 0x00,
    exchange = 0x01,
    iddata   = 0x02,
    data     = 0x03,
    zipdata  = 0x04,
    ping     = 0x05
};

struct __request_type : request_and_reply_base_class {
    enum PackType : byte {
        bad      = (byte)CommonType::bad,     // 0xff
        hello    = (byte)CommonType::hello,   // 0x00
        exchange = (byte)CommonType::exchange,// 0x01
        iddata   = (byte)CommonType::iddata,  // 0x02
        data     = (byte)CommonType::data,    // 0x03
        zipdata  = (byte)CommonType::zipdata, // 0x04
        ping     = (byte)CommonType::ping     // 0x05
    };
}; // struct Engine::__request_type

struct __reply_type : request_and_reply_base_class {
    enum PackType : byte {
        bad      = (byte)CommonType::bad,     // 0xff
        hello    = (byte)CommonType::hello,   // 0x00
        exchange = (byte)CommonType::exchange,// 0x01
        iddata   = (byte)CommonType::iddata,  // 0x02
        data     = (byte)CommonType::data,    // 0x03
        zipdata  = (byte)CommonType::zipdata, // 0x04
        ping     = (byte)CommonType::ping,    // 0x05
        deny     = 0x06,
        timeout  = 0x07
    };
}; // struct Engine::__reply_type


namespace Client {

// Client 用来发给 Server 端的数据包
class request : public __request_type {
public:
    request(void) : pack() {}
    request(byte version,        byte pack_type, 
               data_len_t data_len, const data_t& data)
        : pack(version, pack_type, data_len, data) {}

    request(byte version, byte pack_type,  byte data_len_high, 
               byte data_len_low, const data_t& data)
        : pack(version, pack_type, data_len_high, data_len_low, data) {}

    request(byte version,        byte pack_type, 
               data_len_t data_len, data_t&& data)
        : pack(version, pack_type, data_len, std::move(data)) {}

    request(byte version, byte pack_type,  byte data_len_high, 
               byte data_len_low, data_t&& data)
        : pack(version, pack_type, data_len_high, 
                data_len_low, std::move(data)) {}

    request(const request& that) : pack(that.pack) {}
    request(request&& that) : pack(std::move(that.pack)) {}

    virtual ~request(void) {}

    request& operator= (const request& that) {
        if (this != &that) {
             pack = that.pack;   
        }
        return *this;
    }
    request& operator= (request&& that) {
        if (this != &that) {
            pack = std::move(that.pack);
        }
        return *this;
    }

    request& assign(byte version,   byte pack_type,
            data_len_t data_len, const data_t& data) {
        this->pack = __packet(version, pack_type, data_len, data);
        return *this;
    }
    request& assign(byte version,   byte pack_type,
            data_len_t data_len, data_t&& data) {
        this->pack = __packet(version, pack_type, data_len, std::move(data));
        return *this;
    }

    virtual data_len_t data_len(void) const override {
        data_len_t len = pack.data_len_high_byte;
        return ((len << 8) & 0xff00) | pack.data_len_low_byte;
    }

    data_t& get_data(void) {
        return pack.data;
    }
    const data_t& get_data(void) const {
        return const_cast<request*>(this)->get_data();
    }

    boost::array<boost::asio::const_buffer, 5> buffers(void) const {
        boost::array<boost::asio::const_buffer, 5> bufs = 
        {
            {
                boost::asio::buffer(&(pack.version), 1), 
                boost::asio::buffer(&(pack.type),    1), 
                boost::asio::buffer(&(pack.data_len_high_byte),  1), 
                boost::asio::buffer(&(pack.data_len_low_byte),   1), 
                boost::asio::buffer(pack.data), 
            }
        };
        return bufs;
    }

private:
    __packet pack;
}; // class Engine::Client::request

// Client 端用来接收 Server 端的数据包
class reply : public __reply_type {
public:
    reply(void) : pack(), pack_data_size_setting(false) {}
    explicit reply(const std::size_t data_initial_length) 
        : pack(__packet(0xff, 0xff, 0x00, 0x00, 
                    data_t(data_initial_length, 0))), 
          pack_data_size_setting(true) {} 
    virtual ~reply(void) {}
    
    explicit reply(const reply& that) : pack(that.pack) {}
    explicit reply(reply&& that) : pack(std::move(that.pack)) {}
    explicit reply(const __packet& _pack) : pack(_pack) {}
    explicit reply(__packet&& _pack) : pack(std::move(_pack)) {}
    reply& operator= (const reply& that) {
        if (this != &that) {
             pack = that.pack;   
        }
        return *this;
    }
    reply& operator= (reply&& that) {
        if (this != &that) {
            pack = std::move(that.pack);
        }
        return *this;
    }


    void set_data_size(std::size_t size, byte c = 0) {
        pack.data.resize(size, c);
        pack_data_size_setting = true;
        //_data_size = size;
    }
    void assign_data(std::size_t size, byte c = 0) {
        pack.data.assign(size, c);
        pack_data_size_setting = true;
    }

    // just for reading
    boost::array<boost::asio::mutable_buffer, 5> buffers(void) {
        assert(pack_data_size_setting);
        /*
        pack.version = 0xff;
        pack.type    = 0xff;
        pack.data_len_high_byte = 0x00;
        pack.data_len_high_byte = 0x00;
        pack.data.assign(_data_size, (char)0);
        */
        boost::array<boost::asio::mutable_buffer, 5> bufs =
        {
            {
                boost::asio::buffer(&(pack.version), 1), 
                boost::asio::buffer(&(pack.type),  1), 
                boost::asio::buffer(&(pack.data_len_high_byte),  1), 
                boost::asio::buffer(&(pack.data_len_low_byte),   1), 
                boost::asio::buffer(&(pack.data[0]), pack.data.size()), 
            }
        };
        return bufs;
    }

    byte version(void) {
        return pack.version;
    }

    reply::PackType type(void) {
        return (reply::PackType)pack.type;
    }
    
    virtual data_len_t data_len(void) const override {
        data_len_t len = pack.data_len_high_byte;
        return ((len << 8) & 0xff00) | pack.data_len_low_byte;
    }

    data_t& get_data(void) {
        return pack.data;
    }
    const data_t& get_data(void) const {
        return const_cast<reply*>(this)->get_data();
    }
    
private:
    __packet pack;
    bool pack_data_size_setting = false;
    //std::size_t    _data_size;
}; // class Engine::Client::reply

} // namespace Engine::Client


// namespace Engine::Server
namespace Server {

// Server 端用来接收 Client 端发来的数据包
class request : public __request_type {
public:
    request(void) : pack(), pack_data_size_setting(false) {}
    explicit request(const std::size_t data_initial_length) 
        : pack(__packet(0xff, 0xff, 0x00, 0x00, 
                    data_t(data_initial_length, 0))),
          pack_data_size_setting(true) {} 
    virtual ~request(void) {}

    explicit request(const request& that) : pack(that.pack) {}
    explicit request(request&& that) : pack(std::move(that.pack)) {}
    explicit request(const __packet& _pack) : pack(_pack) {}
    explicit request(__packet&& _pack) : pack(std::move(_pack)) {}
    request& operator= (const request& that) {
        if (this != &that) {
             pack = that.pack;   
        }
        return *this;
    }
    request& operator= (request&& that) {
        if (this != &that) {
            pack = std::move(that.pack);
        }
        return *this;
    }

    void set_data_size(std::size_t size, byte c = 0) {
        pack.data.resize(size, c);
        pack_data_size_setting = true;
        //_data_size = size;
    }
    void assign_data(std::size_t size, byte c = 0) {
        pack.data.assign(size, c);
        pack_data_size_setting = true;
    }

    // just for reading
    boost::array<boost::asio::mutable_buffer, 5> buffers(void) {
        assert(pack_data_size_setting);
        /*
        pack.version = 0xff;
        pack.type    = 0xff;
        pack.data_len_high_byte = 0x00;
        pack.data_len_high_byte = 0x00;
        pack.data.assign(_data_size, (char)0);
        */
        boost::array<boost::asio::mutable_buffer, 5> bufs =
        {
            {
                boost::asio::buffer(&(pack.version), 1), 
                boost::asio::buffer(&(pack.type),  1), 
                boost::asio::buffer(&(pack.data_len_high_byte),  1), 
                boost::asio::buffer(&(pack.data_len_low_byte),   1), 
                boost::asio::buffer(&(pack.data[0]), pack.data.size()), 
            }
        };
        return bufs;
    }

    byte version(void) {
        return pack.version;
    }

    request::PackType type(void) {
        return (request::PackType)pack.type;
    }
    
    virtual data_len_t data_len(void) const override {
        data_len_t len = pack.data_len_high_byte;
        return ((len << 8) & 0xff00) | pack.data_len_low_byte;
    }

    data_t& get_data(void) {
        return pack.data;
    }
    const data_t& get_data(void) const {
        return const_cast<request*>(this)->get_data();
    }
private:
    __packet pack;
    bool pack_data_size_setting = false;
    //std::size_t _data_size;
}; // class Engine::Server::request

// Server 端用来发给 Client 端的数据
class reply : public __reply_type {
public:
    reply(void) : pack() {}
    reply(byte version,        byte pack_type, 
               data_len_t data_len, const data_t& data)
        : pack(version, pack_type, data_len, data) {}

    reply(byte version, byte pack_type,  byte data_len_high, 
               byte data_len_low, const data_t& data)
        : pack(version, pack_type, data_len_high, data_len_low, data) {}

    reply(byte version,        byte pack_type, 
               data_len_t data_len, data_t&& data)
        : pack(version, pack_type, data_len, std::move(data)) {}

    reply(byte version, byte pack_type,  byte data_len_high, 
               byte data_len_low, data_t&& data)
        : pack(version, pack_type, data_len_high, 
                data_len_low, std::move(data)) {}


    reply(const reply& that) : pack(that.pack) {}
    reply(reply&& that) : pack(std::move(that.pack)) {}

    virtual ~reply(void) {}

    reply& operator= (const reply& that) {
        if (this != &that) {
             pack = that.pack;   
        }
        return *this;
    }
    reply& operator= (reply&& that) {
        if (this != &that) {
            pack = std::move(that.pack);
        }
        return *this;
    }


    reply& assign(byte version,   byte pack_type,
            data_len_t data_len, const data_t& data) {
        this->pack = __packet(version, pack_type, data_len, data);
        return *this;
    }
    reply& assign(byte version,   byte pack_type,
            data_len_t data_len, data_t&& data) {
        this->pack = __packet(version, pack_type, data_len, std::move(data));
        return *this;
    }

    virtual data_len_t data_len(void) const override {
        data_len_t len = pack.data_len_high_byte;
        return ((len << 8) & 0xff00) | pack.data_len_low_byte;
    }
    data_t& get_data(void) {
        return pack.data;
    }
    const data_t& get_data(void) const {
        return const_cast<reply*>(this)->get_data();
    }

    boost::array<boost::asio::const_buffer, 5> buffers(void) const {
        boost::array<boost::asio::const_buffer, 5> bufs = 
        {
            {
                boost::asio::buffer(&(pack.version), 1), 
                boost::asio::buffer(&(pack.type),    1), 
                boost::asio::buffer(&(pack.data_len_high_byte),  1), 
                boost::asio::buffer(&(pack.data_len_low_byte),   1), 
                boost::asio::buffer(pack.data), 
            }
        };
        return bufs;
    }

private:
    __packet pack;
}; // class Engine::Server::reply

} // namespace Engine::Server

} // namespace Engine


// function
namespace Engine {

// 包装 packet

namespace Client {

//static inline const request pack_iddata(data_t&& data) {
//    return request(0x00, request::iddata, data.size(), std::move(data));
//} // function Client::pack_iddata
template <class DataType>
static inline const request pack_iddata(DataType&& data) {
    return request(0x00, request::iddata, data.size(),
            std::forward<DataType>(data));
} // function Client::pack_iddata

static inline const request& pack_bad(void) {
    static const request bad(0x00, request::bad, 0x00, data_t());
    return bad;
} // function Client::pack_bad

static inline const request& pack_ping(void) {
    static const request ping(0x00, request::ping, 0x00, data_t());
    return ping;
} // function Client::pack_ping

//static inline const request pack_data(data_t&& data) {
//    return request(0x00, request::data, data.size(), std::move(data));
//} // function Client::pack_data
template <class DataType>
static inline const request pack_data(DataType&& data) {
    return request(0x00, request::data, data.size(),
            std::forward<DataType>(data));
} // function Client::pack_data

} // namespace Engine::Client

namespace Server {
static inline const reply& pack_bad(void) {
    static const reply bad(0x00, reply::bad, 0x00, data_t());
    return bad;
} // function Server::pack_bad

static inline const reply& pack_ping(void) {
    static const reply ping(0x00, reply::ping, 0x00, data_t());
    return ping;
} // function Server::pack_ping

//static inline const reply pack_data(data_t&& data) {
//    return reply(0x00, reply::data, data.size(), std::move(data));
//} // function Server::pack_data

template <class DataType>
static inline const reply pack_data(DataType&& data) {
    return reply(0x00, reply::data, data.size(), std::forward<DataType>(data));
} // function Server::pack_data


} // namespace Engine::Server


// 包完整性检查
static inline void packet_integrity_check(std::size_t bytes_transferred, 
        const request_and_reply_base_class& e_data) 
            throw (Excpt::incomplete_data) {
    if (bytes_transferred < packet::header_size) {
        throw Excpt::incomplete_data(0xffffffff);
    }
    int less = 
        e_data.data_len() + packet::header_size - bytes_transferred;
    if (less > 0) {
        throw Excpt::incomplete_data(less);
    }
}

// 分包 裁剪 e_pack 包数据
// 该方法针对 Server 和 Client 均适用 
/**
 * @brief cut_e_pack
 * @param start_pos  [std::size_t] 开始切割的位置
 * @param e_pack_len    [std::size_t] 当前 e_pack 包有效长度
 * @param e_pack [ Engine::Client::reply | Engine::Server::request ] 要处理的 e_pack 
 * @return  [bool] 切割后剩下后面的数据包是否完整。true 完整，false 不完整 
 */
template<typename ETYPE>
static inline bool cut_e_pack(const std::size_t start_pos, 
        const std::size_t e_pack_len, ETYPE& e_pack) {
    static_assert(std::is_same<ETYPE, Engine::Client::reply>::value ||
            std::is_same<ETYPE, Engine::Server::request>::value,
            "ETYPE must be 'Engine::Client::reply' "
            "or ' Engine::Server::request' !");
    if (e_pack_len < start_pos + packet::header_size) {
        return false;
        // 如果切割后，剩下的数据包会不完整, 所以切割无意义，
        // 多余的丢掉不处理即可。
        // 调用者需检查返回值
    }
    
    vdata_t&& buf = Engine::get_vdata_from_packet(e_pack);
    auto&& pack = Engine::__packet(
            *(buf.begin() + start_pos),
            *(buf.begin() + start_pos + 1),
            *(buf.begin() + start_pos + 2),
            *(buf.begin() + start_pos + 3), // 包头部分结束
            data_t(buf.begin() + start_pos + packet::header_size,
                buf.begin() + e_pack_len)
            ); 
    e_pack = ETYPE(std::move(pack));
    return true;
} // function Engine::cut_e_pack



/////////////////////////////////////////////////////
// shared_type
/////////////////////////////////////////////////////
namespace Server {

// Engine::Server::shared_request_type
// shared_ptr<Server::request>
typedef std::shared_ptr<request> shared_request_type;

// Engine::Server::shared_reply_type
// shared_ptr<Server::reply>
typedef std::shared_ptr<reply>   shared_reply_type;

} // namespace Engine::Server
namespace Client {

// Engine::Client::shared_request_type
// shared_ptr<Client::request>
typedef std::shared_ptr<request> shared_request_type;

// Engine::Client::shared_reply_type
// shared_ptr<Client::reply>
typedef std::shared_ptr<reply>   shared_reply_type;

} // namespace Engine::Client


} // namespace Engine
#endif // E_PACKET_H_1
