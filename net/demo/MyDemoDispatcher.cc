//
// Created by mingweiliu on 2018/11/7.
//

#include "net/demo/MyDemoDispatcher.h"
#include "MyDemoMessage.h"
#include "MyDemoHandler.h"

namespace MF {
    namespace DEMO {
        MyDemoDispatcher::MyDemoDispatcher() {
            //注册handler
            registerHandler<MyUserNameHandler>("username");
            registerHandler<MyPasswordHandler>("password");
            registerHandler<MyQuitHandler>("quit");
        }

        std::string MyDemoDispatcher::getPacketCmd(const char* buf, uint32_t length) {
            std::string msg = std::string(buf, length);
            int32_t pos = static_cast<int32_t >(msg.find(' '));
            std::string cmd;
            if (pos > 0) {
                cmd = msg.substr(0, pos);
            } else {
                if (msg.size() > 0) {
                    cmd = msg.substr(0, msg.size() - 2);
                } else {
                    cmd = msg;
                }
            }

            return cmd;
        }

        int32_t MyDemoDispatcher::dispatchPayload(const std::unique_ptr<Buffer::MyIOBuf>& request,
                                                 std::unique_ptr<Buffer::MyIOBuf> &response
                                                 , std::shared_ptr<Server::MyContext> context) {

            //1. 获取cmd
            std::string cmd = getPacketCmd(static_cast<char*>(request->readable()), request->getReadableLength());

            //2. 找到对应的handler
            auto handler = findHandler(cmd);
            if (handler == nullptr) {
                LOG(ERROR) << "find handler fail, cmd: " << cmd << std::endl;
                return kHandleResultPacketInvalid;
            }

            //2. 解码消息
            std::unique_ptr<Protocol::MyMessage> reqMsg = handler->decode(request);
            if (reqMsg == nullptr) {
                LOG(ERROR) << "decode request message fail, cmd: " << cmd << std::endl;
                return kHandleResultInternalServerError;
            }

            //3. 调用handler
            std::unique_ptr<Protocol::MyMessage> rspMsg = nullptr;
            int32_t rv = handler->doHandler(reqMsg, rspMsg, context);
            if (rv != kHandleResultSuccess) {
                LOG(ERROR) << "handle message fail, cmd: " << cmd << std::endl;
                return rv;
            }

            //4. 编码消息
            if (context->isNeedResponse()) { //需要响应才进行编码
                response = handler->encode(rspMsg);
            }

            return rv;
        }

        int32_t MyDemoDispatcher::handleTimeout(std::shared_ptr<Server::MyContext> context) {
            LOG(INFO) << "client heartbeat timeout" << std::endl;
            return kHandleResultSuccess;
        }

        int32_t MyDemoDispatcher::handleClose(std::shared_ptr<Server::MyContext> context) {
            LOG(INFO) << "client closed" << std::endl;
            return kHandleResultSuccess;
        }
    }
}


