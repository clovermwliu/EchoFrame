//
// Created by mingweiliu on 2018/11/9.
//

#include "MyDemoHandler.h"
#include "MyDemoMessage.h"

namespace MF {
    namespace DEMO {

        int32_t MyUserNameHandler::invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) {
            //打印请求
            LOG(INFO) << "receive name: " << req << std::endl;

            //回复响应
            rsp = "Your name is " + req ;

            return kHandleResultSuccess;
        }

        std::unique_ptr<Protocol::MyMessage> MyUserNameHandler::decode(const std::unique_ptr<Buffer::MyIOBuf>& msg) {
            LOG(INFO) << "MyUserNameHandler::decode" << std::endl;

            auto message = new MyDemoMessage<std::string>();
            //解析cmd
            message->decode(static_cast<char*>(msg->readable()), msg->getReadableLength());

            //解析body
            char* buf = static_cast<char*>(msg->readable()) + message->headLength() + 1; //去掉空格
            uint32_t bodyLen = msg->getReadableLength() - message->headLength() - 2 - 1; //去掉\r\n两个分隔符和空格
            message->setMsg(std::string(buf, bodyLen));

            return std::unique_ptr<MyDemoMessage<std::string> >(message);
        }

        std::unique_ptr<Buffer::MyIOBuf> MyUserNameHandler::encode(const std::unique_ptr<Protocol::MyMessage>& msg) {
            LOG(INFO) << "MyUserNameHandler::encode" << std::endl;

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

            //编码分隔符
            iobuf->write<char*>(const_cast<char*>("\r\n"), 2);

            return std::move(iobuf);
        }

        int32_t MyPasswordHandler::invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) {
            //打印请求
            LOG(INFO) << "receive password: " << req << std::endl;

            //回复响应
            rsp = "Your password is " + req ;
            return kHandleResultSuccess;
        }

        std::unique_ptr<Protocol::MyMessage> MyPasswordHandler::decode(const std::unique_ptr<Buffer::MyIOBuf> &msg) {
            LOG(INFO) << "MyPasswordHandler::decode" << std::endl;

            auto message = new MyDemoMessage<std::string>();
            //解析cmd
            message->decode(static_cast<char*>(msg->readable()), msg->getReadableLength());

            //解析body
            char* buf = static_cast<char*>(msg->readable()) + message->headLength() + 1; //去掉空格
            uint32_t bodyLen = msg->getReadableLength() - message->headLength() - 2 - 1; //去掉\r\n两个分隔符和空格
            message->setMsg(std::string(buf, bodyLen));

            return std::unique_ptr<MyDemoMessage<std::string> > (message);
        }

        std::unique_ptr<Buffer::MyIOBuf> MyPasswordHandler::encode(const std::unique_ptr<Protocol::MyMessage>& msg) {
            LOG(INFO) << "MyPasswordHandler::encode" << std::endl;

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

            //编码分隔符
            iobuf->write<char*>(const_cast<char*>("\r\n"), 2);

            return std::move(iobuf);
        }

        int32_t MyQuitHandler::invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) {
            //打印请求
            LOG(INFO) << "receive quit" << std::endl;

            //回复响应
            rsp = "Connection disconnect now!" + req ;

            //设置不需要响应
            context->setNeedResponse(false);
            //断开链接
            context->close();
            return kHandleResultSuccess;
        }

        std::unique_ptr<Protocol::MyMessage> MyQuitHandler::decode(const std::unique_ptr<Buffer::MyIOBuf> &msg) {
            LOG(INFO) << "MyQuitHandler::decode" << std::endl;

            auto message = new MyDemoMessage<std::string>();
            //解析cmd
            message->decode(static_cast<char*>(msg->readable()), msg->getReadableLength());

            return std::unique_ptr<MyDemoMessage<std::string> > (message);
        }

        std::unique_ptr<Buffer::MyIOBuf> MyQuitHandler::encode(const std::unique_ptr<Protocol::MyMessage> &msg) {
            LOG(INFO) << "MyQuitHandler::encode" << std::endl;

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

            //编码分隔符
            iobuf->write<char*>(const_cast<char*>("\r\n"), 2);

            return std::move(iobuf);
        }
    }
}
