//
// Created by mingweiliu on 2018/11/5.
//

#include "net/server/MyDispatcher.h"

namespace MF {
    namespace Server{
        MyDispatcher::MyDispatcher(MF::Protocol::MyCodec *codec) {
            this->codec = codec;
        }

        int32_t MyDispatcher::handlePacket(const std::unique_ptr<Buffer::MyIOBuf>& req
                , std::shared_ptr<MF::Server::MyContext> context) {

            LOG(INFO) << "MyDispatcher::handlePackets" << std::endl;

            //1. 分发消息
            std::unique_ptr<Buffer::MyIOBuf> rsp;
            int32_t rv;
            if((rv = dispatchPacket(req, rsp, context)) != kHandleResultSuccess) {
                LOG(ERROR) << "dispatchPayload packet fail, close connection" << std::endl;
                context->close();
                return rv;
            }

            //2. 发送响应
            if (context->isNeedResponse() && rsp->getReadableLength() > 0) {
                context->sendPayload(std::move(rsp));
            }
            return rv;
        }

        int32_t MyDispatcher::isPacketComplete(const char *buf, uint32_t length) {
            return codec->isPacketComplete(buf, length);
        }

        uint32_t MyDispatcher::getPacketLength(const char *buf, uint32_t length) {
            return codec->getPacketLength(buf, length);
        }

        void MyDispatcher::setPreFilter(std::unique_ptr<MyFilter> filter) {
            this->preFilter = std::move(filter);
        }

        void MyDispatcher::setPostFilter(std::unique_ptr<MyFilter> filter) {
            this->postFilter = std::move(filter);
        }
    }
}
