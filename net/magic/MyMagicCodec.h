//
// Created by mingweiliu on 2018/11/12.
//

#ifndef MYFRAMEWORK2_MYMAGICCODEC_H
#define MYFRAMEWORK2_MYMAGICCODEC_H


#include "net/protocol/MyCodec.h"
#include "net/protocol/MyMessage.h"

namespace MF {
    namespace MAGIC {

        class MyMagicCodec : public Protocol::MyCodec{
        public:
            int32_t isPacketComplete(const char *buf, uint32_t length) override {
                return Protocol::MyMagicMessage::isPacketComplete(buf, length);
            }

            uint32_t getPacketLength(const char *buf, uint32_t length) override {
                return Protocol::MyMagicMessage::getPacketLength(buf, length);
            }
        };
    }
}


#endif //MYFRAMEWORK2_MYMAGICCODEC_H
