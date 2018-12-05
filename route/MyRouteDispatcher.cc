//
// Created by mingweiliu on 2018/11/30.
//

#include "MyRouteDispatcher.h"

namespace MF {
    namespace Route {

        MyRouteDispatcher::MyRouteDispatcher() {
            registerHandler<MyRegisterHandler>(kCommandCodeRegister);
            registerHandler<MyOperateHandler>(kCommandCodeOperate);
            registerHandler<MyPullTableHandler>(kCommandCodePullTable);
            registerHandler<MyHeartbeatHandler>(kCommandCodeHeartBeat);
        }

        int32_t MyRouteDispatcher::dispatchPayload(const std::unique_ptr<Buffer::MyIOBuf> &request,
                                                   std::unique_ptr<Buffer::MyIOBuf> &response,
                                                   std::shared_ptr<Server::MyContext> context) {

            uint32 cmd = getPacketCmd(reinterpret_cast<char*>(request->readable()), request->getReadableLength());

            //1. 寻找find
            auto handler = findHandler(cmd);
            if (handler == nullptr) {
                LOG(ERROR) << "find handler fail, cmd: " << cmd << std::endl;
                return kHandleResultPacketInvalid;
            }

            //2. 解码数据包
            auto req = handler->decode(request);
            if (req == nullptr) {
                LOG(ERROR) << "decode request message fail, cmd: " << cmd << std::endl;
                return kHandleResultInternalServerError;
            }

            //3.处理数据包
            int32_t rv = kHandleResultSuccess;
            std::unique_ptr<Protocol::MyMessage> rsp(nullptr);
            if((rv = handler->doHandler(req, rsp, context)) != kHandleResultSuccess) {
                LOG(ERROR) << "do handler fail, cmd: " << cmd << std::endl;
                return rv;
            }

            //3. 编码数据包
            if (context->isNeedResponse() && rsp != nullptr) {
                response = std::move(handler->encode(rsp));
            }
            return kHandleResultSuccess;
        }

        uint32_t MyRouteDispatcher::getPacketCmd(const char *buf, uint32_t length) {
            if (buf == nullptr || length < sizeof(uint32_t)) {
                return -1;
            }
            uint32_t cmd = 0;
            memcpy(&cmd, buf, sizeof(uint32_t));
            return cmd;
        }
    }
}