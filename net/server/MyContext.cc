//
//  mycontext.cc
//  MyFramewrok
//
//  Created by mingweiliu on 2017/2/17.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "net/server/MyContext.h"
#include "net/server/EventLoop.h"


namespace MF {
    namespace Server {
        MyContext::MyContext(
                const std::weak_ptr<MyChannel> &channel)
        : channel(channel) {
        }

        void MyContext::sendMessage(const char *buf, uint32_t len) {
            auto c = channel.lock();
            if (c == nullptr || c->sendResponse(buf, len) != len) {
                LOG(ERROR) << "send response fail, len: " << len << std::endl;
            }
        }

        void MyContext::sendMessage(std::unique_ptr<MF::Buffer::MyIOBuf> iobuf) {
            char* buf = static_cast<char*>(iobuf->readable());
            uint32_t length = iobuf->getReadableLength();
            return sendMessage(buf, length);
        }

        void MyContext::close() {
            auto c = channel.lock();
            if (c != nullptr && c->getLoop() != nullptr) {
                auto lp = c->getLoop();
                lp->RunInThread([lp, c] () -> void {
                    lp->removeChannel(c); //删除channel
                });
            }
        }

        bool MyContext::isNeedResponse() const {
            return needResponse;
        }

        void MyContext::setNeedResponse(bool needResponse) {
            MyContext::needResponse = needResponse;
        }
    }
}
