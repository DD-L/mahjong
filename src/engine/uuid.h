#ifndef E_UUID_H_1
#define E_UUID_H_1
/*************************************************************************
	> File Name:    src/engine/uuid.h
	> Author:       D_L
	> Mail:         deel@d-l.top
	> Created Time: 2016/8/19 3:44:13
 ************************************************************************/

#include <engine/typedefine.h>

namespace Engine {

// uuid 增强类

//uuid_t u0(nullptr);
//assert(u0.is_nil());

//uuid_t u1, u2;
//std::cout << u1 << std::endl;
//std::cout << u2 << std::endl;
//
//uuid_t u3("{54dc0207-d958-426c-b32c-f102e326c90b}");
//std::cout << u3 << std::endl;
//std::cout << uuid_t(u3, "test name gen") << std::endl;
//
//
//可能的输出:
//c4f704bc-dfcb-403d-a860-6d21b4d7b6a7
//0243da9d-bbbc-436c-b4a5-8b63e4a7990d
//54dc0207-d958-426c-b32c-f102e326c90b
//52f279a7-a638-5464-aeff-bb68d855a663
//
//
//字符串构造可以接受形如：
//// would like to accept the following forms:
// 0123456789abcdef0123456789abcdef
// 01234567-89ab-cdef-0123456789abcdef
// {01234567-89ab-cdef-0123456789abcdef}
// {0123456789abcdef0123456789abcdef}
//的字符串
//非法字符串会 throw std::runtime_error("invalid uuid string");

class uuid_t : public boost::uuids::uuid {
public:
    using uuid     = boost::uuids::uuid;
    //using nil_uuid = boost::uuids::nil_uuid;
    using random_generator = boost::uuids::random_generator;
    using string_generator = boost::uuids::string_generator;
    using name_generator   = boost::uuids::name_generator;
    // 随机构造
    uuid_t() : uuid(random_generator()()) {}
    // 0 值的 UUID 构造
    explicit uuid_t(std::nullptr_t) : uuid(boost::uuids::nil_uuid()) {};
    // std::string 字符串构造
    explicit uuid_t(const std::string& str) 
        : uuid(string_generator()(str)) {}
    explicit uuid_t(std::string&& str) : uuid_t(str) {}
    explicit uuid_t(const char* str) : uuid(string_generator()(str)) {}
    // std::string 名字生成器构造
    uuid_t(const uuid& u, const std::string& str) :
        uuid(name_generator(u)(str.c_str())) {}
    uuid_t(const uuid& u, std::string&& str) :
        uuid_t(u, str) {}
    // 赋值构造
    explicit uuid_t(const uuid_t& u) : uuid(u) {}
    
    // 转换到 uuid 类型
    operator uuid(void) {
        return static_cast<uuid&>(*this);
    }
    operator uuid(void) const {
        return static_cast<const uuid&>(*this);
    }

    // 转换到字符串
    std::string to_string(void) const {
        return boost::uuids::to_string(*this);
    }
    std::wstring to_wstring(void) const {
        return boost::uuids::to_wstring(*this);
    }

//    // 从父类继承下来的公共成员
//    static size_type static_size(void);
//    size_type size(void) const;
//    uint8_t data[static_size()];
//    
//    iterator begin();
//    iterator end();
//
//    bool is_nil(void) const;
//
//    enum variant_type {
//        variant_ncs,        // NCS backward compatibility
//        variant_nfc_4122,   // difined in RFC 4122 document
//        variant_microsoft,  // Microsoft Coporation backward conoatibility
//        variant_future      // future definition
//    };
//    variant_type variant() const;
//    
//    enum version_type {
//        version_unknown             = -1,
//        version_time_based          = 1,
//        version_dce_security        = 2,
//        version_name_based_md5      = 3,
//        version_random_number_based = 4,
//        version_name_based_sha1     = 5
//    };
//    version_type version() const;
//
//    void swap(uuid& rhs);
}; // class Engine::uuid_t

} // namespace Engine

#endif // E_UUID_H_1
