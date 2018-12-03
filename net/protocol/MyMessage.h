//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYMESSAGE_H
#define MYFRAMEWORK2_MYMESSAGE_H

#include "net/MyGlobal.h"
#include "net/buffer/myIOBuf.h"

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

        /**
         * server 内部使用的协议
         */
        class MyMagicMessage {
        public:
            /**
             * 编码
             * @param payload payload
             */
            virtual std::unique_ptr<Buffer::MyIOBuf> encode();

            /**
             * 解码消息
             * @param payload payload
             */
            virtual void decode(const std::unique_ptr<Buffer::MyIOBuf>& payload);

            virtual uint32_t headLen() const {
                return sizeof(length)
                    + sizeof(version)
                    + sizeof(isRequest)
                    + sizeof(requestId)
                    + sizeof(serverNumber);
            }

            uint32_t getLength() const;

            void setLength(uint32_t length);

            uint16_t getVersion() const;

            void setVersion(uint16_t version);

            int8_t getIsRequest() const;

            void setIsRequest(int8_t isRequest);

            uint64_t getRequestId() const;

            void setRequestId(uint64_t requestId);

            uint32_t getServerNumber() const;

            void setServerNumber(uint32_t serverNumber);

            const std::unique_ptr<Buffer::MyIOBuf> &getPayload() const;

            void setPayload(std::unique_ptr<Buffer::MyIOBuf> payload);


        public:

            /**
             * 解析消息长度
             * @param buf 数据流
             * @param length 数据长度
             * @return 完整数据包长度
             */
            static uint32_t getPacketLength(const char *buf, uint32_t length);

            /**
             * 检查数据包是否完整
             * @param buf buf
             * @param length length
             * @return 检查结果
             */
            static int32_t isPacketComplete(const char* buf, uint32_t length);

        protected:
            uint32_t length; //消息的总长度
            uint16_t version; //协议版本
            int8_t isRequest; //是否请求
            uint64_t requestId; //请求id
            uint32_t serverNumber; //server number

            std::unique_ptr<Buffer::MyIOBuf> payload; //数据包
        };

    }
}



#endif //MYFRAMEWORK2_MYMESSAGE_H
