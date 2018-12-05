//
// Created by mingweiliu on 2018/11/29.
//

#ifndef MYFRAMEWORK2_MYROUTEMESSAGE_H
#define MYFRAMEWORK2_MYROUTEMESSAGE_H

#include "net/protocol/MyMessage.h"
#include "net/buffer/myIOBuf.h"
#include "proto/route.pb.h"
#include "proto/BoxedMessage.h"

namespace MF {
    namespace Route {
        /**
         * 路由消息
         */
        class MyRouteMessage : public Protocol::MyMessage {
        public:
            /**
             * 构造函数
             * @param commandCode
             */
            MyRouteMessage(uint32_t commandCode, std::unique_ptr<Message> payload)
                    : commandCode(commandCode) {
                this->payload = std::move(payload);
            }

            virtual ~MyRouteMessage() {
            }

            /*
             * 获取command code
             */
            uint32_t getCommandCode() const {
                return commandCode;
            }

            /**
             * 获取payload
             * @tparam T 消息类型
             * @return payload
             */
            template<typename T>
            std::unique_ptr<T> getPayload() {
                return std::unique_ptr<T>(dynamic_cast<T*>(payload.release()));
            }

            /**
             * 编码消息
             * @param msg msg
             * @return 码流
             */
            static std::unique_ptr<Buffer::MyIOBuf> encode(const MyRouteMessage* msg) {
                //1. 构造buffer
                std::unique_ptr<Buffer::MyIOBuf> iobuf = Buffer::MyIOBuf::create(msg->getLength());

                //2. 编码cmd
                iobuf->write<uint32_t >(msg->commandCode);
                iobuf->write<std::string>(msg->payload->SerializeAsString());

                return iobuf;
            }

            /**
             * 解码消息
             * @param iobuf iobuf
             * @return 对象
             */
            template<typename T>
            static MyRouteMessage* decode(const std::unique_ptr<Buffer::MyIOBuf>& iobuf) {
                //1. 先解析消息
                uint32_t commandCode = iobuf->read<uint32_t >();
                uint32_t length = iobuf->getReadableLength();
                char* buf = reinterpret_cast<char*>(iobuf->readable());
                auto payload = new T();
                if(!payload->ParseFromArray(buf, length)) {
                    LOG(ERROR) << "decode payload fail, command code: " << commandCode << std::endl;
                    return nullptr;
                }
                iobuf->moveReadable(length);

                //2. 构造消息对象
                auto msg = new MyRouteMessage(commandCode
                        , std::unique_ptr<Message>(dynamic_cast<Message*>(payload)));;
                return msg;
            }
        protected:

            /**
             * 获取数据包长度
             * @return 数据包长度
             */
            uint32_t getLength() const {
                return sizeof(commandCode) + static_cast<uint32_t >(payload->ByteSizeLong());
            }
            uint32_t commandCode; //命令码

            std::unique_ptr<Message> payload; //消息
        };
    }
}


#endif //MYFRAMEWORK2_MYROUTEMESSAGE_H
