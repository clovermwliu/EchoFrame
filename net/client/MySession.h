//
// Created by mingweiliu on 2018/11/15.
//

#ifndef MYFRAMEWORK2_MYSESSION_H
#define MYFRAMEWORK2_MYSESSION_H

#include "util/MyTimeProvider.h"
#include "net/MyGlobal.h"
#include "net/buffer/myIOBuf.h"
#include "net/ev/MyWatcher.h"
#include "net/client/MyClient.h"
#include "net/protocol/MyMessage.h"

namespace MF {
    namespace Client{

        class MyBaseSession: public std::enable_shared_from_this<MyBaseSession>{
        public:
            /**
             * 构造函数
             */
            MyBaseSession() = default;

            /**
             * 析构函数
             */
            virtual ~MyBaseSession() = default;

            uint64_t getRequestId() const {
                return requestId;
            }

            /*
             * 调用成功回调
             * @return 0 成功 其他 失败
             */
            virtual int32_t doSuccessAction(std::unique_ptr<Protocol::MyMessage> response) = 0;

            /**
             * 处理超时回调
             * @return 0成功 其他失败
             */
            virtual int32_t doTimeoutAction() = 0;

            /**
             * 处理失败回调
             * @return 0 成功 其他 失败
             */
            virtual int32_t doErrorAction() = 0;
        protected:
            uint64_t requestId; //请求id
        };

        /**
         * 用于记录一次客户端的调用请求
         */
        template<typename REQ, typename RSP>
        class MySession: public MyBaseSession {
        public:

            //Session中需要记录的参数
            using SuccessAction = std::function<int32_t (
                    const std::unique_ptr<RSP>&, const std::shared_ptr<MySession<REQ,RSP>>&)>;

            using FailAction = std::function<int32_t (
                    const std::shared_ptr<MySession<REQ,RSP>>&)>;

            MySession(std::weak_ptr<MyClient> client);

            virtual ~MySession();

            /**
            * 请求完成后需要做的事情
            * @param pred 需要做的事情
            * @param args 调用的参数
            * @return proxy自身
            */
            template<typename Pred, typename... Args>
            MySession<REQ, RSP>& then(Pred&& pred, Args... args) {
                this->thenAction = std::bind(pred, std::placeholders::_1, std::placeholders::_2, args...);
                return *this;
            }

            /**
             * 超时之后要做的事情
             * @param pred 需要调用的函数
             * @param args 参数列表
             * @return proxy自生
             */
            template<typename Pred, typename... Args>
            MySession<REQ, RSP>& timeout(Pred&& pred, Args... args) {
                this->timeoutAction = std::bind(pred, std::placeholders::_1, args...);
                return *this;
            }

            /**
             * 失败之后要做的事情
             * @param pred 需要调用的函数
             * @param args 参数列表
             * @return proxy自生
             */
            template<typename Pred, typename... Args>
            MySession<REQ, RSP>& error(Pred&& pred, Args... args) {
                this->errorAction = std::bind(pred, std::placeholders::_1, args...);
                return *this;
            }

            /**
             * 发送数据
             */
            void execute();

            /**
             * 执行并等待
             * @return 响应
             */
            std::unique_ptr<RSP> executeAndWait();

            /**
             * 设置request
             * @param request request
             */
            void setRequest(std::unique_ptr<Protocol::MyMagicMessage> request);

            void setTimeoutWatcher(EV::MyTimerWatcher *timeoutWatcher);

            const std::unique_ptr<Buffer::MyIOBuf> &getRequest() const;

            int32_t doSuccessAction(std::unique_ptr<Protocol::MyMessage> response) override;

            int32_t doTimeoutAction() override;

            int32_t doErrorAction() override;

        private:
            SuccessAction thenAction; //成功回调
            FailAction timeoutAction; //超时回调
            FailAction errorAction; //错误回调

            std::weak_ptr<MyClient> client; //客户端指针

            EV::MyTimerWatcher* timeoutWatcher {nullptr}; //超时watcher

            std::unique_ptr<Protocol::MyMagicMessage> request; //请求内容, 包含头信息

            std::unique_ptr<RSP> response; //响应内容，不包含头信息

            bool isAsync{true}; //是否异步

            std::promise<int32_t > promise; //返回的响应
        };

        template<typename REQ, typename RSP>
        MySession<REQ, RSP>::MySession(std::weak_ptr<MF::Client::MyClient> client) : MyBaseSession() {
            this->client = client;
            auto c = this->client.lock();
            if (c != nullptr) {
                this->requestId = (static_cast<uint64_t >(c->getUid()) << 32) | MyTimeProvider::now();
            }
        }

        template<typename REQ, typename RSP>
        void MySession<REQ, RSP>::execute() {
            auto c = client.lock();
            if (c == nullptr) {
                LOG(ERROR) << "connection closed" << std::endl;
                return ;
            }
            auto r = request->encode();
            if(c->sendPayload(std::move(r)) != 0) {
                LOG(ERROR) << "send request fail, uid: " << c->getUid()
                           << ", requestId: " << getRequestId() << std::endl;
                return;
            }

            LOG(INFO) << "send request success, uid: " << c->getUid()
                      << ", requestId: " << getRequestId() << std::endl;

            //增加定时器用于检查是否超时
            auto self = std::weak_ptr<MyBaseSession>(shared_from_this());
            c->whenSessionTimeout(std::bind([self]() -> void {
                auto s = std::dynamic_pointer_cast<MySession<REQ, RSP>>(self.lock());
                if (s != nullptr) {
                    s->doTimeoutAction();
                }
            }), this->requestId);
        }

        template <typename REQ, typename RSP>
        std::unique_ptr<RSP> MySession<REQ, RSP>::executeAndWait() {
            isAsync = false; //设置为同步
            //发送数据
            auto c = client.lock();
            if (c == nullptr) {
                LOG(ERROR) << "connection closed" << std::endl;
                return nullptr;
            }
            auto r = request->encode();
            if(c->sendPayload(std::move(r)) != 0) {
                LOG(ERROR) << "send request fail, uid: " << c->getUid()
                        << ", requestId: " << getRequestId() << std::endl;
            } else {
                LOG(INFO) << "send request success, uid: " << c->getUid()
                        << ", requestId: " << getRequestId() << std::endl;
            }

            //生成future
            auto future = promise.get_future(); //生成future
            if (!future.valid()) {
                return nullptr;
            }

            //等待响应
            auto rv = future.wait_for(std::chrono::seconds(c->getConfig().timeout));
            if (rv != std::future_status::ready) {
                //ready
                LOG(ERROR) << "wait for response fail, requestId: " << requestId << std::endl;
                return nullptr;
            }
            
            //检查响应是否处理成功
            if (future.get() != kClientResultSuccess) {
                LOG(ERROR) << "client result error, requestId: " << requestId << ", code: " << future.get() << std::endl;
                return nullptr;
            }
            return std::move(response);
        }

        template<typename REQ, typename RSP>
        MySession<REQ, RSP>::~MySession() {
            if (timeoutWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(timeoutWatcher);
            }
        }

        template<typename REQ, typename RSP>
        void MySession<REQ, RSP>::setTimeoutWatcher(EV::MyTimerWatcher *timeoutWatcher) {
            this->timeoutWatcher = timeoutWatcher;
        }

        template<typename REQ, typename RSP>
        const std::unique_ptr<Buffer::MyIOBuf> &MySession<REQ, RSP>::getRequest() const {
            return request;
        }

        template<typename REQ, typename RSP>
        void MySession<REQ, RSP>::setRequest(std::unique_ptr<Protocol::MyMagicMessage> request) {
            this->request = std::move(request);
        }

        template<typename REQ, typename RSP>
        int32_t MySession<REQ, RSP>::doSuccessAction(std::unique_ptr<Protocol::MyMessage> response) {
            int32_t rv = kClientResultSuccess;
            this->response = std::unique_ptr<REQ>((REQ*)(response.release()));
            if (isAsync) {
                //异步
                if (thenAction) {
                    auto self = std::dynamic_pointer_cast<MySession<REQ, RSP>>(shared_from_this());
                    rv = thenAction(this->response, self);
                }
            } else {
                //同步, 发送通知
                promise.set_value(kClientResultSuccess);
            }

            return rv;
        }

        template<typename REQ, typename RSP>
        int32_t MySession<REQ, RSP>::doTimeoutAction() {
            int32_t rv = kClientResultSuccess;
            if (isAsync) {
                //异步
                if (timeoutAction) {
                    auto self = std::dynamic_pointer_cast<MySession<REQ, RSP>>(shared_from_this());
                    rv = timeoutAction(self);
                }
            } else {
                //同步, 发送通知
                promise.set_value(kClientResultTimeout);
            }

            return rv;
        }

        template<typename REQ, typename RSP>
        int32_t MySession<REQ, RSP>::doErrorAction() {
            int32_t rv = kClientResultSuccess;
            if (isAsync) {
                //异步
                if (errorAction) {
                    auto self = std::dynamic_pointer_cast<MySession<REQ, RSP>>(shared_from_this());
                    rv = errorAction(self);
                }
            } else {
                //同步, 发送通知
                promise.set_value(kClientResultFail);
            }

            return rv;
        }
    }
}


#endif //MYFRAMEWORK2_MYSESSION_H
