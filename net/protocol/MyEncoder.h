//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYENCODER_H
#define MYFRAMEWORK2_MYENCODER_H


#include "net/MyGlobal.h"
#include "net/buffer/myIOBuf.h"
#include "MyMessage.h"

namespace MF {
    namespace Protocol {
        class MyEncoder {

        public:
            /**
             * 编码消息, buf 内存有encoder分配，不需要释放
             * @param buf buffer
             * @param message message
             * @return buffer 长度 =0 失败
             */
            virtual uint32_t invoke(char** buf, MyMessage* message) = 0;

        };
    }
}



#endif //MYFRAMEWORK2_MYENCODER_H
