//
// Created by mingweiliu on 2018/11/30.
//

#include "route/MyRouteProxy.h"
#include "proto/route.pb.h"
#include "route/MyRouteMessage.h"
#include "net/client/ClientLoop.h"

namespace MF {
    namespace Route {

        MyRouteProxy::MyRouteProxy(Client::ClientLoopManager *loops) : ServantProxy(loops) {}

        std::unique_ptr<RegisterRsp> MyRouteProxy::registerServant(std::unique_ptr<RegisterReq> req) {
            //1. 编码消息
            auto reqMsg = std::unique_ptr<MyRouteMessage>(new MyRouteMessage(kCommandCodeRegister, std::move(req)));

            //2. 构造request
            auto rspMsg = buildSession<MyRouteMessage, MyRouteMessage>(std::move(reqMsg))->executeAndWait();
            if (rspMsg == nullptr) {
                LOG(ERROR) << "register servant fail" << std::endl;
                return nullptr;
            }

            //3. 返回响应
            return rspMsg->getPayload<RegisterRsp>();
        }

        std::unique_ptr<OperateRsp> MyRouteProxy::setServantStatus(std::unique_ptr<OperateReq> req) {
            //1. 编码消息
            auto reqMsg = std::unique_ptr<MyRouteMessage>(new MyRouteMessage(kCommandCodeOperate, std::move(req)));

            //2. 构造request
            auto rspMsg = buildSession<MyRouteMessage, MyRouteMessage>(std::move(reqMsg))->executeAndWait();
            if (rspMsg == nullptr) {
                LOG(ERROR) << "set servant status fail" << std::endl;
                return nullptr;
            }

            //3. 返回响应
            return rspMsg->getPayload<OperateRsp>();
        }

        std::unique_ptr<Heartbeat> MyRouteProxy::heartbeat(std::unique_ptr<Heartbeat> req) {
            //1. 编码消息
            auto reqMsg = std::unique_ptr<MyRouteMessage>(new MyRouteMessage(kCommandCodeHeartBeat, std::move(req)));

            //2. 构造request
            auto rspMsg = buildSession<MyRouteMessage, MyRouteMessage>(std::move(reqMsg))->executeAndWait();
            if (rspMsg == nullptr) {
                LOG(ERROR) << "send heartbeat fail" << std::endl;
                return nullptr;
            }

            //3. 返回响应
            return rspMsg->getPayload<Heartbeat>();
        }

        int32_t MyRouteProxy::isPacketComplete(const char *buf, uint32_t len) {
            return Protocol::MyMagicMessage::isPacketComplete(buf, len);
        }

        uint32_t MyRouteProxy::getPacketLength(const char *buf, uint32_t len) {
            return Protocol::MyMagicMessage::getPacketLength(buf, len);
        }

        std::unique_ptr<Protocol::MyMessage> MyRouteProxy::decode(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) {
            MyRouteMessage* m = nullptr;
            //1. 解析前4个字节
            uint32_t cmd = iobuf->read<uint32_t >();
            if (cmd == kCommandCodeRegister) {
                m = MyRouteMessage::decode<RegisterReq>(iobuf);
            } else if (cmd == kCommandCodeOperate) {
                m = MyRouteMessage::decode<OperateReq>(iobuf);
            } else if (cmd == kCommandCodePullTable) {
                m = MyRouteMessage::decode<PullTableReq>(iobuf);
            } else if (cmd == kCommandCodeHeartBeat) {
                m = MyRouteMessage::decode<Heartbeat>(iobuf);
            }
            return std::unique_ptr<MyRouteMessage>(m);
        }

        std::unique_ptr<Buffer::MyIOBuf> MyRouteProxy::encode(const std::unique_ptr<Protocol::MyMessage> &message) {
            //1. 解析前4个字节
            return MyRouteMessage::encode(dynamic_cast<MyRouteMessage*>(message.get()));
        }
    }
}
