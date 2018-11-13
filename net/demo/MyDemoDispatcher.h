//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYDEMODISPATCHER_H
#define MYFRAMEWORK2_MYDEMODISPATCHER_H

#include "net/server/MyDispatcher.h"
#include "MyDemoMessage.h"
#include "net/demo/MyDemoHandler.h"

namespace MF {
    namespace DEMO {
        class MyDemoDispatcher : public Server::MyDispatcher{
        public:
            /**
             * 构造函数
             * @param codec 编解码器
             */
            MyDemoDispatcher(Protocol::MyCodec *codec);

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

        public:
            int32_t handleTimeout(std::shared_ptr<Server::MyContext> context) override;

            int32_t handleClose(std::shared_ptr<Server::MyContext> context) override;

        protected:

            /**
             * 注册handler
             */
            template<typename HANDLER>
            void registerHandler(const std::string& cmd) {
                auto handler = new HANDLER();
                handlers[cmd] = handler;
            }

            /**
             * 获取handler
             * @tparam HANDLER_TYPE
             * @param cmd cmd
             * @return handler
             */
            MyDemoHandler<std::string, std::string>* findHandler(const std::string& cmd) {
                return handlers.find(cmd) != handlers.end() ? handlers[cmd] : nullptr;
            }


            /**
             * 获取数据包的cmd
             * @param msg 消息
             * @return cmd
             */
            std::string getPacketCmd(const char* buf, uint32_t length);

            std::map<std::string, MyDemoHandler<std::string, std::string>*> handlers; //handlers
        };
    }
}



#endif //MYFRAMEWORK2_MYDEMODISPATCHER_H
