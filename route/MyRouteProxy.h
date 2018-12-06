//
// Created by mingweiliu on 2018/11/30.
//

#ifndef MYFRAMEWORK2_MYROUTEPROXY_H
#define MYFRAMEWORK2_MYROUTEPROXY_H

#include "net/client/MyProxy.h"
#include "proto/route.pb.h"

namespace MF {
    namespace Route {
        class MyRouteProxy : public Client::ServantProxy{
        public:
            MyRouteProxy(Client::ClientLoopManager *loops);

            /**
             * 注册servant
             * @param req 请求
             * @return 结果
             */
            std::unique_ptr<RegisterRsp> registerServant(std::unique_ptr<RegisterReq> req);

            /**
             * 修改servant状态
             * @param req req
             * @return 结果
             */
            std::unique_ptr<OperateRsp> setServantStatus(std::unique_ptr<OperateReq> req);

            /**
             * 心跳操作
             * @return 心跳响应
             */
            std::unique_ptr<Heartbeat> heartbeat(std::unique_ptr<Heartbeat> req);
        protected:

            int32_t isPacketComplete(const char *buf, uint32_t len) override;

            uint32_t getPacketLength(const char *buf, uint32_t len) override;

            unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) override;

            unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage> &message) override;

        private:
        };
    }
}


#endif //MYFRAMEWORK2_MYROUTEPROXY_H
