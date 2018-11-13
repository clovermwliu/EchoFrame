//
// Created by mingweiliu on 2018/11/8.
//

#ifndef MYFRAMEWORK2_MYFILTER_H
#define MYFRAMEWORK2_MYFILTER_H


#include "net/MyGlobal.h"
#include "net/protocol/MyMessage.h"
#include "net/server/MyContext.h"
namespace  MF {

    namespace Server {

        /**
         * 过滤器基类, 倒叙执行，最先加入的过滤器最先执行
         * new MyFilter3(new MyFilter2(new MyFilter1()))
         * 执行顺序为Filter1 --> Filter2 --> Filter3
         */
        class MyFilter {
        public:
            /**
             * 构造函数
             * @param nextFilter nextFilter
             */
            MyFilter(MyFilter *nextFilter);

            /**
             * 构造函数
             */
            MyFilter();

            /**
             * 析构函数
             */
            virtual ~MyFilter();

            /**
             * 调用入口
             * @param request 请求
             * @param context context
             * @return true 继续 false 停止
             */
            bool doFilter(Protocol::MyMessage* request, std::shared_ptr<MyContext> context);

            /**
             * 过滤消息
             * @param request 请求消息
             * @param context 请求的上下文
             * @return true 继续执行 false 中断流程
             */
            virtual bool invoke(Protocol::MyMessage* request, std::shared_ptr<MyContext> context) = 0;

        protected:
            MyFilter* nextFilter; //下一个过滤器
        };
    }
}


#endif //MYFRAMEWORK2_MYFILTER_H
