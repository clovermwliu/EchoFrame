//
// Created by mingweiliu on 2018/11/30.
//

#ifndef MYFRAMEWORK2_BOXEDPROTOBUFMESSAGE_H
#define MYFRAMEWORK2_BOXEDPROTOBUFMESSAGE_H

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include "util/MyCommon.h"

using namespace ::google::protobuf;

namespace MF {
    namespace ProtoBuf {
        template<typename PAYLOAD>
        class BoxedMessage {
        public:
            /**
             * 构造函数
             * @param payload payload
             */
            explicit BoxedMessage(const PAYLOAD& payload) {
                this->payload = payload;
            }

            BoxedMessage() {}

            /**
             * 析构函数
             */
            virtual ~BoxedMessage() = default;

            /**
             * 重载箭头指针
             * @return
             */
            PAYLOAD* operator -> () {
                return &payload;
            }

        protected:
            PAYLOAD payload;
        };
    }
}


#endif //MYFRAMEWORK2_BOXEDPROTOBUFMESSAGE_H
