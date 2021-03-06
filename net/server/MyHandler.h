//
// Created by mingweiliu on 2018/11/9.
//

#ifndef MYFRAMEWORK2_MYHANDLER_H
#define MYFRAMEWORK2_MYHANDLER_H

#include "net/MyGlobal.h"
#include "net/buffer/myIOBuf.h"
#include "net/protocol/MyMessage.h"
#include "net/server/MyContext.h"

namespace MF {
    namespace Server {
        /**
         * Handler基类
         */
        class MyHandler {

        public:

            /**
             * 析构函数
             */
            virtual ~MyHandler() = default;

            /**
             * 执行handler
             */
            virtual int32_t doHandler(
                    const std::unique_ptr<Protocol::MyMessage>& request
                    , std::unique_ptr<Protocol::MyMessage>& response
                    , std::shared_ptr<Server::MyContext> context)  = 0;

            /**
             * 解码消息
             * @param msg 消息
             * @return 对象
             */
            virtual std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf>& msg) = 0;

            /**
             * 编码消息
             * @param msg 对象
             * @return 消息
             */
            virtual std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage>& msg) = 0;
        };
    }
}


#endif //MYFRAMEWORK2_MYHANDLER_H
