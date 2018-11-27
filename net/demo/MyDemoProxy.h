//
// Created by mingweiliu on 2018/11/16.
//

#ifndef MYFRAMEWORK2_MYDEMOPROXY_H
#define MYFRAMEWORK2_MYDEMOPROXY_H

#include "net/client/MyProxy.h"


namespace MF {
    namespace DEMO {
        /**
         * demo proxy
         */
        class MyDemoProxy : public Client::ServantProxy{
        public:
            MyDemoProxy(Client::ClientLoopManager *loops);

            /**
             * 设置用户名
             * @param username username
             * @return 结果
             */
            std::string setUsername(const std::string& username);

            /**
             * 设置password
             * @param password password
             * @return  结果
             */
            std::string setPassword(const std::string& password);

            /**
             * 断开链接
             */
            void quit();

        protected:
            int32_t isPacketComplete(const char *buf, uint32_t len) override;

            uint32_t getPacketLength(const char *buf, uint32_t len) override;

            unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) override;

            unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage> &message) override;

        protected:
        };
    }
}


#endif //MYFRAMEWORK2_MYDEMOPROXY_H
