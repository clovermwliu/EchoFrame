//
// Created by mingweiliu on 2018/11/7.
//

#ifndef MYFRAMEWORK2_MYDEMOMESSAGE_H
#define MYFRAMEWORK2_MYDEMOMESSAGE_H

#include "net/protocol/MyMessage.h"
#include "net/buffer/myIOBuf.h"

namespace MF {
    namespace DEMO {

        class MyDemoHead : public Protocol::MyMessage {
        public:
            MyDemoHead(const std::string& cmd) {
                this->cmd = cmd;
            }

            MyDemoHead() {}

            const std::string &getCmd() const {
                return cmd;
            }

            /**
             * 获取消息头的长度
             */
            uint32_t headLength() const {
                return static_cast<uint32_t >(cmd.length());
            }

            void decode(const char* buf, uint32_t length) {
                std::string m = std::string(buf, length);
                int32_t pos = m.find(' ');
                if (pos > 0) {
                    cmd = m.substr(0, pos);
                } else {
                    if (m.size() > 2) {
                        cmd = m.substr(0, m.size() - 2);
                    } else {
                        cmd = m;
                    }
                }
            }

            /**
             * 编码
             */
            void encode(std::unique_ptr<Buffer::MyIOBuf>& iobuf) {
                iobuf->write<char*>(const_cast<char*>(cmd.c_str()), static_cast<uint32_t >(cmd.size()));
            }

        protected:
            std::string cmd;
        };

        /**
         * Demo消息
         */
        template<typename BODY>
        class MyDemoMessage : public MyDemoHead{
        public:
            MyDemoMessage(const std::string& cmd, const BODY& msg)
            : MyDemoHead(cmd){
                this->msg = msg;
            };

            MyDemoMessage() {}

            const BODY &getMsg() const {
                return msg;
            };

            void setMsg(const BODY msg) {
                MyDemoMessage::msg = msg;
            }

            uint32_t length(uint32_t bodyLen) const {
                return headLength() + 1 + bodyLen + 2;
            }
        protected:
            BODY msg;
        };
    }
}



#endif //MYFRAMEWORK2_MYDEMOMESSAGE_H
