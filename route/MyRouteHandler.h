//
// Created by mingweiliu on 2018/11/30.
//

#ifndef MYFRAMEWORK2_MYROUTEHANDLER_H
#define MYFRAMEWORK2_MYROUTEHANDLER_H

#include "route/MyRouteMessage.h"
#include "net/server/MyHandler.h"

namespace MF {
    namespace Route{

        /**
         * 不同消息的处理对象
         * @tparam REQ
         * @tparam RSP
         */
        template<typename REQ, typename RSP>
        class MyRouteHandler : public Server::MyHandler{
        public:
            std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf> &msg) override {
                auto m = MyRouteMessage::decode<REQ>(msg);
                return std::unique_ptr<Protocol::MyMessage>(m);
            }


            std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage> &msg) override {
                return MyRouteMessage::encode(dynamic_cast<MyRouteMessage*>(msg.get()));
            }
        };

        class MyRegisterHandler : public MyRouteHandler<RegisterReq, RegisterRsp> {
        public:
            int32_t doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                              std::unique_ptr<Protocol::MyMessage> &response,
                              std::shared_ptr<Server::MyContext> context) override;
        };

        class MyOperateHandler : public MyRouteHandler<OperateReq, OperateRsp> {
        public:
            int32_t doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                              std::unique_ptr<Protocol::MyMessage> &response,
                              std::shared_ptr<Server::MyContext> context) override;
        };

        class MyPullTableHandler : public MyRouteHandler<PullTableReq, PullTableRsp> {
        public:
            int32_t doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                              std::unique_ptr<Protocol::MyMessage> &response,
                              std::shared_ptr<Server::MyContext> context) override;
        };

        class MyHeartbeatHandler : public MyRouteHandler<Heartbeat, Heartbeat> {
        public:
            int32_t doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                              std::unique_ptr<Protocol::MyMessage> &response,
                              std::shared_ptr<Server::MyContext> context) override;
        };
    }
}



#endif //MYFRAMEWORK2_MYROUTEHANDLER_H
