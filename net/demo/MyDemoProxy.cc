//
// Created by mingweiliu on 2018/11/16.
//

#include "net/demo/MyDemoProxy.h"
#include "net/demo/MyDemoMessage.h"

namespace MF {
    namespace DEMO {

        using DemoMsg = std::unique_ptr<MyDemoMessage<std::string>>;
        using DemoContext = std::shared_ptr<Client::MyRequest<MyDemoMessage<std::string>, MyDemoMessage<std::string>>>;

        MyDemoProxy::MyDemoProxy(
                Client::ClientLoopManager *loops)
                : ServantProxy(loops) {

        }

        std::string MyDemoProxy::setUsername(const std::string &username) {

            auto req = std::move(std::unique_ptr<MyDemoMessage<std::string>>(
                    new MyDemoMessage<std::string>("username", username)));
            auto rsp = this->buildRequest<
                    MyDemoMessage<std::string>, MyDemoMessage<std::string>
                            >(std::move(req))->executeAndWait();
            if (rsp == nullptr) {
                LOG(INFO) << "wait for response fail, cmd: username " << username << std::endl;
                return "";
            }
            return rsp->getMsg();
        }

        std::string MyDemoProxy::setPassword(const std::string &password) {
            auto req = std::move(std::unique_ptr<MyDemoMessage<std::string>>(
                    new MyDemoMessage<std::string>("password", password)));
            auto rsp = this->buildRequest<
                    MyDemoMessage<std::string>, MyDemoMessage<std::string>
                            >(std::move(req))->executeAndWait();
            if (rsp == nullptr) {
                LOG(INFO) << "wait for response fail, cmd: password " << password << std::endl;
                return "";
            }
            return rsp->getMsg();
        }

        void MyDemoProxy::quit() {
            auto req = std::move(std::unique_ptr<MyDemoMessage<std::string>>(
                    new MyDemoMessage<std::string>("quit", "")));
            this->buildRequest<
                    MyDemoMessage<std::string>, MyDemoMessage<std::string>
            >(std::move(req))->executeAndWait();
        }

        int32_t MyDemoProxy::isPacketComplete(const char *buf, uint32_t len) {
            return Protocol::MyMagicMessage::isPacketComplete(buf, len);
        }

        uint32_t MyDemoProxy::getPacketLength(const char *buf, uint32_t len) {
            return Protocol::MyMagicMessage::getPacketLength(buf, len);
        }

        unique_ptr<Protocol::MyMessage> MyDemoProxy::decode(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) {
            std::unique_ptr<MyDemoMessage<std::string>> message(new MyDemoMessage<std::string>());

            LOG(INFO) << "MyDemoProxy::decode" << std::endl;

            //解析cmd
            message->decode(static_cast<char*>(iobuf->readable()), iobuf->getReadableLength());

            //解析body
            char* buf = static_cast<char*>(iobuf->readable()) + message->headLength() + 1; //去掉空格
            uint32_t bodyLen = iobuf->getReadableLength() - message->headLength() - 1; //去掉\r\n两个分隔符和空格
            message->setMsg(std::string(buf, bodyLen));

            return message;
        }

        unique_ptr<Buffer::MyIOBuf> MyDemoProxy::encode(const std::unique_ptr<Protocol::MyMessage> &msg) {
            LOG(INFO) << "MyDemoProxy::encode" << std::endl;

            auto message = dynamic_cast<MyDemoMessage<std::string>* >(msg.get());
            std::unique_ptr<Buffer::MyIOBuf> iobuf = Buffer::MyIOBuf::create(
                    message->length(static_cast<uint32_t >(message->getMsg().size())));
            // 编码头部
            message->encode(iobuf);

            // 分隔符
            iobuf->write<char>(' ');

            // 编码body
            iobuf->write<char*>(
                    const_cast<char*>(message->getMsg().c_str())
                    , static_cast<uint32_t >(message->getMsg().size()));

            return std::move(iobuf);
        }
    }
}
