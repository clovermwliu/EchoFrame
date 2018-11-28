//
//  mysocket.h
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mysocket_h
#define mysocket_h

//socket相关的头文件
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

//tcp相关文件
#include "netinet/in.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>

#include <string>

//自定义头文件
#include "util/MyException.h"
namespace MF {
    namespace Socket {
        /// socket基础类
        class MySocket {
        public:
            /**
             *  @brief 构造函数
             */
            MySocket() = default;
            
            /**
             *  @brief 析构函数
             */
            virtual ~MySocket();

            /**
             * 构造socket
             * @param domain domain
             * @param type type
             * @param protocol protocol
             * @return 0 成功 其他错误码
             */
            int32_t socket(uint8_t domain, int32_t type, int32_t protocol);

            /**
             *  @brief 绑定一个本地地址
             *
             *  @param host 本机ip
             *  @param port 本地端口
             *
             */
            int32_t bind(const std::string& host, uint16_t port);
            
            /**
             *  @brief 绑定一个端口, ip使用0.0.0.0(绑定所有网卡)
             *
             *  @param port 本地端口
             *
             */
            int32_t bind(uint16_t port);
            
            /**
             *  @brief 监听端口
             *
             *  @param backlog 最大accept排队数量
             *
             */
            int32_t listen(uint32_t backlog);
            
            /**
             *  @brief 连接到一个端口
             *
             *  @param host 对端ip
             *  @param port 对端端口
             *
             */
            int32_t connect(const std::string& host, uint16_t port);

            /**
             * 获取connect结果
             * @return 0 成功 其他 失败
             */
            int32_t getConnectResult() const ;
            
            /**
             *  @brief 接受一个新的连接
             *
             *  @param socket 新的连接
             */
            int32_t accept(MySocket* socket);
            
            /**
             *  @brief 重用地址
             */
            void setReuseAddr(bool flag) {
                setSockOpt(SOL_SOCKET, SO_REUSEADDR, flag ? 1 : 0);
            }
            
            /**
             *  @brief port重用
             */
            void setReusePort(bool flag) {
                setSockOpt(SOL_SOCKET, SO_REUSEPORT, flag ? 1 : 0);
            }
            /**
             *  @brief 设置保活
             */
            void setKeepAlive(bool flag) {
                setSockOpt(SOL_SOCKET, SO_KEEPALIVE, flag ? 1 : 0);
            }
            
            /**
             *  @brief 设置linger
             */
            void setLinger(bool flag, int32_t timeout) {
                struct linger l = {flag ? 1 : 0, timeout};
                ::setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)(&l), sizeof(struct linger));
            }
            
            /**
             *  @brief 设置广播权限
             */
            void setBroadcast(bool flag) {
                setSockOpt(SOL_SOCKET, SO_BROADCAST, flag ? 1 : 0);
            }
            /**
             *  @brief 设置发送窗口大小
             *
             *  @param length 窗口大小(字节)
             */
            void setSendBuf(uint32_t length) {
                setSockOpt(SOL_SOCKET, SO_SNDBUF, (int32_t) length);
            }
            
            /**
             *  @brief 设置接收窗口大小
             *
             *  @param length 窗口大小(字节)
             */
            void setRecvBuf(uint32_t length) {
                setSockOpt(SOL_SOCKET, SO_RCVBUF, (int32_t) length);
            }
            
            /**
             *  @brief 设置最小发送字节叔
             *
             *  @param length 字节数
             */
            void setSendLowWait(uint32_t length) {
                setSockOpt(SOL_SOCKET, SO_SNDLOWAT, (int32_t) length);
            }
            
            /**
             *  @brief 设置最小接收字节数
             *
             *  @param length 字节数
             */
            void setRecvLowWait(uint32_t length) {
                setSockOpt(SOL_SOCKET, SO_RCVLOWAT, (int32_t) length);
            }
            
            /**
             *  @brief 设置发送超时时间
             *
             *  @param timeout 超时时长(ms)
             */
            void setSendTimeout(uint32_t timeout) {
                setSockOpt(SOL_SOCKET, SO_SNDTIMEO, (int32_t) timeout);
            }
            
            /**
             *  @brief 设置接收超时时长
             *
             *  @param timeout 时长 ms
             */
            void setRecvTimeout(uint32_t timeout) {
                setSockOpt(SOL_SOCKET, SO_RCVTIMEO, (int32_t) timeout);
            }
            
            /**
             *  @brief 设置socket为block模式
             */
            void setBlock() {
                auto flags = fcntl(fd, F_GETFD, 0);
                fcntl(fd, F_SETFD, flags & (~O_NONBLOCK));
            }
            
            /**
             *  @brief 设置socket为nonblock模式
             */
            void setNonBlock() {
                auto flags = fcntl(fd, F_GETFL, 0);
                fcntl(fd, F_SETFL, flags|O_NONBLOCK);
            }
            
            /**
             *  @brief 关闭socket
             */
            void close() {
                if ( fd != 0) {
                    ::close(fd);
                    setClosed();
                    fd = 0;
                }
            }
            
            /**
             *  @brief 写入一段数据。 写入失败时抛出异常
             *
             *  @param buffer 数据
             *  @param length 数据长度
             */
            int32_t write(void* buffer, uint32_t length);
            
            /**
             *  @brief 发送数据 UDP, 失败时抛出异常
             *
             *  @param host   对端ip
             *  @param port   对端端口
             *  @param buffer buffer
             *  @param length 长度
             *
             *  @return 写入的数据长度
             */
            int32_t writeTo(const std::string &host, uint16_t port, void *buffer, uint32_t length);
            
            /**
             *  @brief 发送数据 UDP, 失败时抛出异常
             *
             *  @param addr     对端地址
             *  @param addr_len 对端地址长度
             *  @param buffer   buffer
             *  @param length   buffer长度
             *
             *  @return 写入的数据长度
             */
            int32_t writeTo(struct sockaddr *addr, void *buffer, uint32_t length);

            /**
             *  @brief 读取一段数据
             *
             *  @param buffer 数据buffer
             *  @param size   buffer容量
             *
             *  @return 读取到的字节数
             */
            int32_t read(void* buffer, uint32_t size);
            
            /**
             *  @brief 从某个socket读取消息
             *
             *  @param addr     对端的地址， 出错时抛出异常
             *  @param addr_len 地址长度
             *  @param buffer   buffer
             *  @param size     buffer容量
             *
             *  @return 读取到的数据长度
             */
            int32_t readFrom(struct sockaddr *addr, socklen_t *addr_len, void *buffer, uint32_t size);

            /**
             * 查看消息来源
             * @param addr addr
             * @param addrLen addrlen
             * @return 消息的长度
             */
            int32_t peekFrom(struct sockaddr* addr, socklen_t *addrLen);
            
            /**
             *  @brief 获取对端的ip
             *
             *  @return 对端ip
             */
            const std::string& getRemoteHost() const {
                return remote.host;
            }
            
            /**
             *  @brief 获取对端端口
             *
             *  @return 对端端口
             */
            uint16_t getRemotePort() const {
                return remote.port;
            }
            
            /**
             *  @brief 获取fd
             *
             *  @return fd
             */
            int32_t getfd() const {
                return fd;
            }
            
            /**
             *  @brief 查看是否已经连接
             *
             *  @return true 已连接 false 未连接
             */
            bool connected() const {
                return isConnected;
            }
            
            /**
             *  @brief 设置socket已连接
             */
            void setConnected() {
                isConnected = true;
                isClosed = false;
            }
            
            void setClosed() {
                isConnected = false;
                isClosed = true;
            }
            
        protected:
            /**
             *  @brief 设置socket的属性
             *
             *  @param level       level
             *  @param option_name opt
             *  @param value       value
             *
             */
            void setSockOpt(int32_t level, int32_t option_name, int32_t value);
            
            int32_t fd {0}; //描述符
            struct {
                std::string host; //ip
                uint16_t port {0}; //端口
            }remote;
            
            struct {
                std::string host; //ip
                uint16_t port {0}; //端口
            }local_;
            
            bool isConnected {false}; //是否已经连接
            bool isClosed {true}; //是否已经关闭
            
            uint8_t domain {0};
            int32_t type {0};
            int32_t protocol {0};
        };
    }
}
#endif /* mysocket_h */
