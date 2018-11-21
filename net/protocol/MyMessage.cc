//
// Created by mingweiliu on 2018/11/7.
//

#include "net/protocol/MyMessage.h"

namespace MF {
    namespace Protocol {

        std::unique_ptr<Buffer::MyIOBuf> MyMagicMessage::encode() {
            std::unique_ptr<Buffer::MyIOBuf> payload(Buffer::MyIOBuf::create(length));
            payload->write<uint32_t >(length);
            payload->write<uint16_t >(version);
            payload->write<int8_t >(isRequest);
            payload->write<uint64_t >(requestId);
            payload->write<uint32_t >(serverNumber);

            //写入数据体
            payload->write<void*>(
                    this->payload->readable()
                    , this->payload->getReadableLength());

            return payload;
        }

        void MyMagicMessage::decode(const std::unique_ptr<MF::Buffer::MyIOBuf> &payload) {
            length = payload->read<uint32_t >();
            version = payload->read<uint16_t >();
            isRequest = payload->read<int8_t >();
            requestId = payload->read<uint64_t >();
            serverNumber = payload->read<uint32_t >();

            this->payload = Buffer::MyIOBuf::create(length - headLen());
            this->payload->write<void*>(payload->readable(), payload->getReadableLength());
        }

        uint32_t MyMagicMessage::getLength() const {
            return length;
        }

        void MyMagicMessage::setLength(uint32_t length) {
            MyMagicMessage::length = length;
        }

        uint16_t MyMagicMessage::getVersion() const {
            return version;
        }

        void MyMagicMessage::setVersion(uint16_t version) {
            MyMagicMessage::version = version;
        }

        int8_t MyMagicMessage::getIsRequest() const {
            return isRequest;
        }

        void MyMagicMessage::setIsRequest(int8_t isRequest) {
            MyMagicMessage::isRequest = isRequest;
        }

        uint64_t MyMagicMessage::getRequestId() const {
            return requestId;
        }

        void MyMagicMessage::setRequestId(uint64_t requestId) {
            MyMagicMessage::requestId = requestId;
        }

        uint32_t MyMagicMessage::getServerNumber() const {
            return serverNumber;
        }

        void MyMagicMessage::setServerNumber(uint32_t serverNumber) {
            MyMagicMessage::serverNumber = serverNumber;
        }

        const std::unique_ptr<Buffer::MyIOBuf> &MyMagicMessage::getPayload() const {
            return payload;
        }

        void MyMagicMessage::setPayload(std::unique_ptr<Buffer::MyIOBuf> payload) {
            MyMagicMessage::payload = std::move(payload);
        }
    }
}

