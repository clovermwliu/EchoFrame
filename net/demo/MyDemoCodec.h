//
// Created by mingweiliu on 2018/11/12.
//

#ifndef MYFRAMEWORK2_MYDEMOCODEC_H
#define MYFRAMEWORK2_MYDEMOCODEC_H


#include "net/protocol/MyCodec.h"

namespace MF {
    namespace DEMO {

        class MyDemoCodec : public Protocol::MyCodec{
        public:
            int32_t isPacketComplete(const char *buf, uint32_t length) override {
                return std::string(buf, length).find("\r\n") > 0 ? kPacketStatusComplete : kPacketStatusIncomplete;
            }

            uint32_t getPacketLength(const char *buf, uint32_t length) override {
                return static_cast<uint32_t >(std::string(buf, length).find("\r\n") + 2);
            }
        };
    }
}


#endif //MYFRAMEWORK2_MYDEMOCODEC_H
