//
// Created by mingweiliu on 2018/11/6.
//

#ifndef MYFRAMEWORK2_MYCODEC_H
#define MYFRAMEWORK2_MYCODEC_H

#include <functional>
#include <memory>
#include <net/buffer/myIOBuf.h>
#include "net/MyGlobal.h"
#include "net/protocol/MyMessage.h"
#include "MyEncoder.h"
#include "MyDecoder.h"

namespace MF {
    namespace Protocol {
        class MyCodec {
        public:
            virtual ~MyCodec() = default;

            /**
             * 判断包是否完整
             * @param buf buf
             * @param length length
             * @return 检查结果
             */
            virtual int32_t isPacketComplete(const char* buf, uint32_t length) = 0;

            /**
             * 获取数据包的长度
             * @param buf buf
             * @param length length
             * @return 数据包长度
             */
            virtual uint32_t getPacketLength(const char* buf, uint32_t length) = 0;

        protected:
        };
    }
}




#endif //MYFRAMEWORK2_MYCODEC_H
