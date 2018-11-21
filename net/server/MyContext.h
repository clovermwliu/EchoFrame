//
//  mycontext.h
//  MyFramewrok
//  Created by mingweiliu on 2017/2/17.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mycontext_h
#define mycontext_h

#include <memory>
#include "net/MyGlobal.h"
#include "net/server/MyChannel.h"

namespace MF {
    namespace Server {
        
        /// 一个连接，提供读写接口
        class MyContext {
        public:
            /**
             * 构造函数
             * @param channel 连接
             */
            explicit MyContext(const std::weak_ptr<MyChannel> &channel);

            /**
             * 发送响应
             * @param buf buffer
             * @param len length
             */
            void sendPayload(const char *buf, uint32_t len);

            /**
             * 发送响应
             * @param iobuf iobuf
             */
            void sendPayload(std::unique_ptr<Buffer::MyIOBuf> iobuf);

            /**
             * 关闭连接
             */
            void close();

            /**
             * 获取channel
             * @return channel
             */
            std::shared_ptr<MyChannel> getChannel() const {
                return channel.lock();
            }

            bool isNeedResponse() const;

            void setNeedResponse(bool needResponse);

        protected:
            std::weak_ptr<MyChannel> channel; //用于标识一个连接

            bool needResponse {true}; //是否需要返回响应
        };
    }
}

#endif /* mychannel_h */
