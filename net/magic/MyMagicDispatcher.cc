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

            //2. 分发数据
            std::unique_ptr<Buffer::MyIOBuf> rspBuf;
            auto rv = dispatchPayload(reqMsg->getPayload(), rspBuf, context);
            if (rv != kHandleResultSuccess) {
                LOG(ERROR) << "dispatch payload fail, uid: "
                << context->getChannel()->getUid()
                << ", requestId: " << reqMsg->getRequestId() << std::endl;
                return rv;
            }

            //3. 编码消息
            auto rspMsg = std::move(std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage()));
            rspMsg->setLength(rspMsg->headLen() + rspBuf->getReadableLength());
            rspMsg->setRequestId(reqMsg->getRequestId());
            rspMsg->setServerNumber(reqMsg->getServerNumber());
            rspMsg->setIsRequest(0);
            rspMsg->setVersion(reqMsg->getVersion());
            rspMsg->setPayload(std::move(rspBuf));

            response = std::move(rspMsg->encode());
            return rv;
        }
    }
}

