//
// Created by mingweiliu on 2018/11/5.
//

#ifndef MYFRAMEWORK2_MYDISPATCHER_H
#define MYFRAMEWORK2_MYDISPATCHER_H

#include <map>
#include "net/MyGlobal.h"
#include "net/protocol/MyCodec.h"
#include "net/server/MyContext.h"
#include "net/server/MyFilter.h"
#include "net/server/MyHandler.h"

namespace MF {
    namespace Server {

        class MyDispatcher {
        public:
            /**
             * 构造函数
             * @param codec dispatcher的编解码器
             */
            MyDispatcher(Protocol::MyCodec* codec);

            /**
             * 处理数据包
             * @param context context
             */
            virtual int32_t handlePacket(
                    const std::unique_ptr<Buffer::MyIOBuf>& iobuf, std::shared_ptr<MyContext> context);

            /**
             * 处理链接超时
             * @param context 上下文
             */
            virtual int32_t handleTimeout(std::shared_ptr<MyContext> context) {return kHandleResultSuccess;}

            /**
             * 处理对端关闭连接
             * @param context context
             */
            virtual int32_t handleClose(std::shared_ptr<MyContext> context) {return kHandleResultSuccess;}

            /**
             * 数据包是否完整
             * @param buf buffer
             * @param length 数据长度
             * @return 检查结果
             */
            int32_t isPacketComplete(const char* buf, uint32_t length);

            /**
             * 获取数据包的长度
             * @param buf buffer
             * @param length buffer length
             * @return 完整数据包的长度
             */
            uint32_t getPacketLength(const char* buf, uint32_t length);

            /**
             * 设置前置Filter
             * @param filter
             */
            void setPreFilter(std::unique_ptr<MyFilter> filter);

            /**
             * 设置后置Filter
             * @param postFilter
             */
            void setPostFilter(std::unique_ptr<MyFilter> filter);

        protected:
            /**
             * 分发数据包
             * @param message message
             * @return 0 成功 其他失败
             */
            virtual int32_t dispatchPacket(
                    const std::unique_ptr<Buffer::MyIOBuf>& request
                    , std::unique_ptr<Buffer::MyIOBuf>& response
                    , std::shared_ptr<MyContext> context) = 0;

            Protocol::MyCodec* codec; //不同类型消息的编解码器

            std::unique_ptr<MyFilter> preFilter{nullptr}; //前置过滤器

            std::unique_ptr<MyFilter> postFilter{nullptr}; //后置过滤器
        };
    }
}


#endif //MYFRAMEWORK2_MYDISPATCHER_H
