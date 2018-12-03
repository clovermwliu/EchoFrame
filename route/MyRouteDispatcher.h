//
// Created by mingweiliu on 2018/11/30.
//

#ifndef MYFRAMEWORK2_MYROUTEDISPATCHER_H
#define MYFRAMEWORK2_MYROUTEDISPATCHER_H

#include "route/MyRouteHandler.h"
#include "net/magic/MyMagicDispatcher.h"

namespace MF {
    namespace Route {
        class MyRouteDispatcher : public MAGIC::MyMagicDispatcher{
        protected:
            /**
             * 分发数据包
             * @param request
             * @param response
             * @param context
             * @return 分发结果
             */
            int32_t dispatchPayload(
                    const std::unique_ptr<Buffer::MyIOBuf> &request
                    , std::unique_ptr<Buffer::MyIOBuf> &response
                    , std::shared_ptr<Server::MyContext> context) override;

        public:

            MyRouteDispatcher();

            /**
             * 注册handler
             */
            template<typename HANDLER>
            void registerHandler(uint32_t cmd) {
                auto handler = new HANDLER();
                handlers[cmd] = handler;
            }

            /**
             * 获取handler
             * @tparam HANDLER_TYPE
             * @param cmd cmd
             * @return handler
             */
            Server::MyHandler* findHandler(uint32_t cmd) {
                return handlers.find(cmd) != handlers.end() ? handlers[cmd] : nullptr;
            }


            /**
             * 获取数据包的cmd
             * @param msg 消息
             * @return cmd
             */
            uint32_t getPacketCmd(const char* buf, uint32_t length);

        protected:

            std::map<uint32 , Server::MyHandler*> handlers; //handlers
        };
    }
}



#endif //MYFRAMEWORK2_MYROUTEDISPATCHER_H
