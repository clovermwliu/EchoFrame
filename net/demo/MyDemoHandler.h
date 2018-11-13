//
// Created by mingweiliu on 2018/11/9.
//

#ifndef MYFRAMEWORK2_MYDEMOHANDLER_H
#define MYFRAMEWORK2_MYDEMOHANDLER_H


#include "net/MyGlobal.h"
#include "net/demo/MyDemoMessage.h"
#include "net/buffer/myIOBuf.h"
#include "net/server/MyHandler.h"
#include "net/server/MyContext.h"

namespace MF {
    namespace DEMO {
        template <typename REQ, typename RSP>
        class MyDemoHandler : public Server::MyHandler{
        public:
            virtual ~MyDemoHandler() {

            }

            /**
             * 处理请求
             * @param request request
             * @param response response
             * @return 处理结果
             */
            int32_t doHandler(
                    const std::unique_ptr<Protocol::MyMessage>& request
                    , std::unique_ptr<Protocol::MyMessage>& response
                    , std::shared_ptr<Server::MyContext> context) {
                auto req = dynamic_cast<MyDemoMessage<REQ>*>(request.get());
                REQ reqBody = req->getMsg();
                RSP rspBody;

                //处理请求
                int32_t rv = invoke(reqBody, rspBody, context);
                response = std::unique_ptr<Protocol::MyMessage>(new MyDemoMessage<RSP>(req->getCmd(), rspBody));
                return rv;
            }

        protected:

            /**
             * 实际调用的
             * @param req
             * @param rsp
             * @return
             */
            virtual int32_t invoke(const REQ& req, RSP& rsp, std::shared_ptr<Server::MyContext> context) = 0;
        };

        //userName
        class MyUserNameHandler : public MyDemoHandler<std::string, std::string> {
        protected:
        private:
            int32_t invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) override;

            std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf>& msg) override;

            std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage>& msg) override;
        };

        //password
        class MyPasswordHandler: public MyDemoHandler<std::string, std::string> {
        protected:
        private:
            int32_t invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) override;

            std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf> &msg) override;

            std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage> &msg) override;
        };

        //quit
        class MyQuitHandler : public MyDemoHandler<std::string, std::string> {
        protected:
        private:
            int32_t invoke(const std::string &req, std::string &rsp, std::shared_ptr<Server::MyContext> context) override;

            std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf>& msg) override;

            std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage>& msg) override;
        };
    }
}


#endif //MYFRAMEWORK2_MYDEMOHANDLER_H
