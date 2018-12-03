//
//  myglobal.h
//  MyFramewrok
//  全局定义
//  Created by mingweiliu on 2017/2/15.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myglobal_h
#define myglobal_h

#include <iostream>
#include "util/MyCommon.h"

namespace MF {
    typedef enum enumPacketStatus : int32_t  {
        kPacketStatusIncomplete = 0, //不完整
        kPacketStatusComplete = 1, //完整
        kPacketStatusError = 99, //错误
    }PacketStatus;

    typedef enum enumHandleResult : int32_t  {
        kHandleResultSuccess = 0, //成功
        kHandleResultClose = 1, //关闭连接
        kHandleResultPacketInvalid = 100, //包错误
        kHandleResultInternalServerError = 101, //Server错误
    }HandleResult;

    typedef enum enumClientResult : int32_t  {
        kClientResultSuccess = 0, //成功
        kClientResultTimeout = 1, //超时
        kClientResultFail = 100, //失败
    }ClientResult;

    //socket相关
    const uint32_t g_default_listen_backlog = 10; //默认等待链接数
    
    const uint32_t g_default_skbuffer_capacity = 1024 * 16; //socket使用的skbuffer的默认长度
    const uint32_t g_default_iobuf_capacity = 1024; //iobuf使用的skbuffer默认长度
    const uint32_t g_max_packet_length = 1024 * 1024; //默认的最大数据包长度
    const uint32_t g_default_wheel_timer_slot_count = 1024 * 1024; //最大slot个数

    const uint32_t g_max_udp_packet_length = 1500; //udp最大数据包长度

    
    typedef enum EndpointProtocol : uint32_t {
        kTCP = 0,
        kUDP = 1,
        kUNIX = 2,
    }EndpointProtocol;
    
    typedef struct MyEndpoint {
        EndpointProtocol protocol_ {kTCP}; //协议类型
        std::string name_; //服务名称
        uint32_t timeout_{3000}; //超时时长
        std::string host_; //监听的host
        uint16_t port_{0}; //监听端口
        
        bool isvalid() const {
            return !(name_.empty() || protocol_ < kTCP || protocol_ > kUNIX);
        }
    }MyEndpoint;
    
    //tcp -h 127.0.0.1 -p 12345 -n test_server -t 3000
    class MyEndpointHelper {
    public:
        /**
         *  @brief string 转endpoint
         *
         *  @param str 字符串
         *
         *  @return endpoint
         */
        static MyEndpoint StringToEndpoint(const std::string& str);
        
        /**
         *  @brief endpoint 转string
         *
         *  @param ep endpoint
         *
         *  @return 字符串
         */
        static std::string EndpointToString(const MyEndpoint& ep);
    protected:
    private:
    };
    
    //App配置
    typedef struct AppConfig {
        std::vector<std::string> eps_; //所有需要监听的servant
        std::string server_name_; //服务名
        uint32_t iothread_; //io线程数
        uint32_t netthread_; //网络线程数
        uint32_t sync_timeout_; //同步调用超时时长(ms)
        uint32_t async_timeout_; //异步调用超时时长(ms)
        
        AppConfig() {
            iothread_ = 1;
            netthread_ = 1;
            sync_timeout_ = 3000;
            async_timeout_ = 5000;
        }
    }AppConfig;
    
    
}
#endif /* myglobal_h */
