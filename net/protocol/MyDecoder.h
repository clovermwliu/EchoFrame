//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYDECODER_H
#define MYFRAMEWORK2_MYDECODER_H

#include "net/MyGlobal.h"
#include "net/protocol/MyMessage.h"

namespace MF {
    namespace Protocol {

        class MyDecoder {
        public:
            /**
             * 不同协议的解码器
             * @param buffer buffer
             * @param length length
             * @return 解码出来的消息
             */
            virtual MyMessage* invoke(const char *buffer, uint32_t length) = 0;

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


#endif //MYFRAMEWORK2_MYDECODER_H
