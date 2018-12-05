//
// Created by mingweiliu on 2018/11/7.
//

#include "net/magic/MyMagicDispatcher.h"

namespace MF {
    namespace MAGIC{
        MyMagicDispatcher::MyMagicDispatcher()
        : MyDispatcher(new MyMagicCodec()) {
        }

        MyMagicDispatcher::~MyMagicDispatcher() {
            if (codec != nullptr) {
                delete(codec);
            }
        }

        int32_t MyMagicDispatcher::dispatchPacket(const std::unique_ptr<Buffer::MyIOBuf>& request,
                                                 std::unique_ptr<Buffer::MyIOBuf> &response
                                                 , std::shared_ptr<Server::MyContext> context) {

            //1. 解码消息
            auto reqMsg = std::move(std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage()));
            reqMsg->decode(request);

            //检查是否心跳消息
            if (reqMsg->isHeartbeat()) {
                LOG(INFO) << "receive heartbeat message, requestId: " << reqMsg->getRequestId() << std::endl;
                //返回心跳响应
                reqMsg->setIsRequest(0); //设置为响应
                response = std::move(reqMsg->encode());
                return kHandleResultSuccess;
            }

            //2. 分发数据
            std::unique_ptr<Buffer::MyIOBuf> rspBuf;
            auto rv = dispatchPayload(reqMsg->getPayload(), rspBuf, context);
            if (rv != kHandleResultSuccess) {
                LOG(ERROR) << "dispatch payload fail, requestId: " << reqMsg->getRequestId() << std::endl;
                return rv;
            }

            //3. 编码消息
            if (context->isNeedResponse()) { //需要回复响应
                auto rspMsg = std::move(std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage()));
                rspMsg->setRequestId(reqMsg->getRequestId());
                rspMsg->setServerNumber(reqMsg->getServerNumber());
                rspMsg->setIsRequest(0);
                rspMsg->setVersion(reqMsg->getVersion());

                if (rspBuf != nullptr) {
                    rspMsg->setLength(rspMsg->headLen() + rspBuf->getReadableLength());
                    rspMsg->setPayload(std::move(rspBuf));
                } else {
                    rspMsg->setLength(rspMsg->headLen());
                }
                response = std::move(rspMsg->encode());
            }
            return rv;
        }
    }
}


