# mahjong [![Build Status](https://travis-ci.org/DD-L/mahjong.svg?branch=develop)](https://travis-ci.org/DD-L/mahjong)
这是一款基于 asio 并使用现代 C++ 编程风格编写的轻量级(游戏)服务器引擎，客户端通过简单的约定协议就能与服务端交互。

#### 目标
  * 目标 1：简单易用，不仅仅针对网络游戏；
  * 目标 2：使得开发者无需重复的实现一些(游戏)服务端通用的底层技术，将精力真正集中到应用(游戏)开发层面上来，快速的打造各种网络应用(游戏)；
  
  * 目标 3：高并发、高性能、高可用。

#### Engine 配置文件
 * [server-config.json](./src/engine/server-config.json)

#### 现已完成的
  目前完成的工作有限:
  
  * 引擎部分
    * 异步事件驱动器
      * asio 异步事件驱动
    * 底层通信封装
      * 网络I/O
      * 底层协议
      * heartbeat/keepalive
      * EServer
    * 客户端会话管理
      * 基于uuid 的 session
      * session_manager
  * [mahjong demo](./src/mahjong)
    * connection
    * session
    * handler_test
  * 客户端网络通信共享库
    * 与客户端的 C# 交互
  * 其他
    * 日志库
    * program_options
    * json/config
    * exceptions
    * ...

#### 尚未结束的
  * Unity3D demo（35%）
  * DBMgr
  * UsersMgr
  * Python Script（对热更新的支持）
  * docker 支持
  * crypto
  * udp
  * ...
  
#### 该项目网络部分和其它一小部分借鉴了 [lproxy](https://github.com/DD-L/lproxy)
