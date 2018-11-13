//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYMESSAGE_H
#define MYFRAMEWORK2_MYMESSAGE_H

#include "net/MyGlobal.h"

namespace MF {
    namespace Protocol {
        /**
         * 消息的基类
         */
        class MyMessage {
        public:
            virtual ~MyMessage() {}

        protected:
        };
    }
}



#endif //MYFRAMEWORK2_MYMESSAGE_H
