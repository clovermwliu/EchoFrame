//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYMAGICDISPATCHER_H
#define MYFRAMEWORK2_MYMAGICDISPATCHER_H

#include "net/server/MyDispatcher.h"
#include "net/protocol/MyMessage.h"
#include "net/magic/MyMagicCodec.h"

namespace MF {
    namespace MAGIC {
        class MyMagicDispatcher : public Server::MyDispatcher{
        public:
            /**
             * 构造函数
             * @param codec 编解码器
             */
            MyMagicDispatcher();

            /**
             * 析构函数
             */
            virtual ~MyMagicDispatcher();

        protected:
            /**
             * 分发数据包
             * @param request request
             * @param response repsonse
             * @return 分发结果
             */
            int32_t dispatchPacket(const std::unique_ptr<Buffer::MyIOBuf>& request,
                                   std::unique_ptr<Buffer::MyIOBuf> &response
                                   , std::shared_ptr<Server::MyContext> context) override;
            /**
             * 分发收到的数据内容
             * @param request request
             * @param response response
             * @param context context
             * @return 分发结果
             */
            virtual int32_t dispatchPayload(const std::unique_ptr<Buffer::MyIOBuf> &request,
                                            std::unique_ptr<Buffer::MyIOBuf> &response,
                                            std::shared_ptr<Server::MyContext> context) = 0;

        protected:
        };
    }
}



#endif //MYFRAMEWORK2_MYMAGICDISPATCHER_H
