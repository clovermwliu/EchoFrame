//
//  myevwatcher.h
//  MF
//
//  Created by mingweiliu on 17/1/22.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myevwatcher_h
#define myevwatcher_h

#include <stdio.h>
#include <cstdint>
#include <functional>
#include <string>
#include <iostream>
#include <ev.h>
#include <glog/logging.h>
#include <set>

#include "util/MyFactory.h"
#include "util/MySingleton.h"

namespace MF {
    namespace EV {
        class MyWatcherManager;

        //公用基类
        class MyWatcher {
        public:
            MyWatcher() = default;
            virtual ~MyWatcher() = default;
            typedef std::function<void(MyWatcher*)> COB; //事件发生是需要执行的函数
            friend class MyWatcherManager; //声明友元
            
            /**
             *  @brief 将一个watcher 添加到loop中
             *
             *  @param loop 事件循环
             */
            virtual void append(struct ev_loop* loop) = 0;
            
            /**
             *  @brief 从loop中删除一个watcher
             *
             */
            virtual void subtract() = 0;
            
            /**
             *  @brief 判断watcher是否有效
             *
             *  @return true 有效 false 无效
             */
            bool is_valid() const {
                return is_valid_;
            }
            
            /**
             *  @brief 判断是否已经被监听
             *
             *  @return true 已监听 false 未监听
             */
            bool is_listened() const {
                return is_listened_;
            }
            
            /**
             *  @brief 获取eventloop
             *
             *  @return event looop
             */
            struct ev_loop* loop() const {
                return event_.loop;
            }
            
            /**
             *  @brief 获取event事件
             *
             *  @return event事件
             */
            int32_t events() const {
                return event_.revents;
            }

        private:
            //创建一个对象
            template<typename T>
            static T* create(COB&& pred) {
                return new T(std::move(pred));
            }
            
            //删除一个对象
            static void destroy(MyWatcher* watcher) {
                //先停止监听
                if (watcher->is_listened()) {
                    watcher->subtract();
                }
                delete watcher;
                watcher = nullptr;
            }
        protected:
            bool is_valid_ {false}; //是否有效
            bool is_listened_ {false}; //是否已经被监听
            COB callback_;
            
            struct {
                struct ev_loop* loop {nullptr}; //触发的线程循环
                int32_t revents; //事件
            }event_;
        };
        
        //TODO: 暂时只支持ev_io ev_timer ev_signal 和ev_idle四种事件，其他的后续支持
        /// 基础的事件
        template<typename T>
        class MyWatcherBase : public MyWatcher{
        public:
            
            static bool value;
            
            MyWatcherBase(COB && cb) {
                callback_ = cb;
                watcher_.data = (void*)this;
            }
            
            /**
             *  @brief 判断是否是一个watcher类型
             *
             *  @return true 是
             */
            static bool iswatcher() {
                return true;
            }
            
            /**
             *  @brief watcher的回调函数
             *
             *  @param loop    事件循环
             *  @param watcher 监控事件
             *  @param revents 发生的事情
             */
            static void callback(struct ev_loop* loop, T* watcher, int32_t revents) {
                //1. 先获取watcher
                MyWatcherBase<T>* selfptr = (MyWatcherBase<T>*)(watcher->data);
                if (selfptr && selfptr->callback_) { //找到了watcher自己，并且callback可执行
                    selfptr->event_.loop = loop;
                    selfptr->event_.revents = revents;
                    selfptr->callback_(dynamic_cast<MyWatcher*>(selfptr));
                }
            }
            
            /**
             *  @brief 获取watcher
             *
             *  @return watcher指针
             */
            const T* watcher() const {
                return &watcher_;
            }

            uint32_t getUid() const {
                return uid;
            }

            void setUid(uint32_t uid) {
                MyWatcherBase::uid = uid;
            }

        protected:
            T watcher_; //事件

            uint32_t uid{0}; //链接id
        private:
        };
        
        template<typename T> bool MyWatcherBase<T>::value = true;
        
        //io事件
        class MyIOWatcher : public MyWatcherBase<ev_io> {
        public:
            
            /**
             *  @brief 构造 watcher
             */
            MyIOWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_io>(std::move(pred)){
                ev_init(&watcher_, callback);
            }
            
            /**
             *  @brief 设置IO事件
             *
             *  @param fd      fd
             *  @param revents 监听的时间
             */
            MyIOWatcher* set(int32_t fd, int32_t revents) {
                ev_io_set(&watcher_, fd, revents);
                is_valid_ = true;
                return this;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_io_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_io_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
        protected:
        private:
        };
        
        //timer事件
        class MyTimerWatcher : public MyWatcherBase<ev_timer> {
        public:
            /**
             *  @brief 构造 watcher
             */
            MyTimerWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_timer>(std::move(pred)){
                ev_init(&watcher_, callback);
            }
            
            /**
             *  @brief 设置timer事件
             *
             *  @param after  第一次执行的等待时长
             *  @param repeat 重复执行的间隔
             */
            MyTimerWatcher* set(ev_tstamp after, ev_tstamp repeat) {
                ev_timer_set(&watcher_, after, repeat);
                is_valid_ = true;
                return this;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_timer_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_timer_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
        protected:
        private:
        };
        
        //signal事件
        class MySignalWatcher : public MyWatcherBase<ev_signal> {
        public:
            /**
             *  @brief 构造 watcher
             */
            MySignalWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_signal>(std::move(pred)){
                ev_init(&watcher_, callback);
            }
            
            /**
             *  @brief 设置信号事件
             *
             *  @param signal 信号
             */
            MySignalWatcher* set(int32_t signal) {
                ev_signal_set(&watcher_, signal);
                is_valid_ = true;
                return this;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_signal_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_signal_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
            
            void signal() {
                ev_feed_signal(watcher()->signum);
            }
        protected:
        private:
        };
        
        //signal事件
        class MyStatWatcher : public MyWatcherBase<ev_stat> {
        public:
            
            /**
             *  @brief 构造 watcher
             */
            MyStatWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_stat>(std::move(pred)){
                ev_init(&watcher_, callback);
            }
            
            /**
             *  @brief 设置stat事件
             *
             *  @param path     监听的目录
             *  @param interval 监听的间隔
             */
            MyStatWatcher* set(const std::string& path, ev_tstamp interval) {
                ev_stat_set(&watcher_, path.c_str(), interval);
                is_valid_ = true;
                return this;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_stat_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_stat_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
        protected:
        private:
            std::string path_;
        };
        
        //idle事件
        class MyIdleWatcher : public MyWatcherBase<ev_idle> {
        public:
            /**
             *  @brief 构造 watcher
             */
            MyIdleWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_idle>(std::move(pred)){
                ev_idle_init(&watcher_, callback);
                is_valid_ = true;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_idle_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_idle_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
        protected:
        private:
        };
        
        //async事件
        class MyAsyncWatcher : public MyWatcherBase<ev_async> {
        public:
            /**
             *  @brief 构造 watcher
             */
            MyAsyncWatcher(MyWatcher::COB && pred)
            : MyWatcherBase<ev_async>(std::move(pred)){
                ev_async_init(&watcher_, callback);
                is_valid_ = true;
            }
            
            //添加到事件循环
            void append(struct ev_loop* loop) override {
                ev_async_start(loop, &watcher_);
                is_listened_ = true;
                event_.loop = loop;
            }
            
            //从循环删除事件
            void subtract() override {
                if (event_.loop) {
                    ev_async_stop(event_.loop, &watcher_);
                    is_listened_ = false;
                }
            }
            
            //做出通知
            void signal() {
                if (event_.loop) { //如果loop不为null才做通知
                    ev_async_send(event_.loop, &watcher_);
                }
            }
        protected:
        private:
        };
        
        /// watcher对象的管理器
        class MyWatcherManager : public MySingleton<MyWatcherManager>{
        public:
            //watcher的工厂定义
            typedef MyPointerFactory<std::string, MyWatcher*, MyWatcher::COB> MyWatcherFactory;
            
            /**
             *  @brief 构造函数, 构造时注册所有类型的watcher
             */
            MyWatcherManager() {
                RegisterObject<MyIOWatcher>(MAKE_NAME(MyIOWatcher));
                RegisterObject<MyTimerWatcher>(MAKE_NAME(MyTimerWatcher));
                RegisterObject<MySignalWatcher>(MAKE_NAME(MySignalWatcher));
                RegisterObject<MyStatWatcher>(MAKE_NAME(MyStatWatcher));
                RegisterObject<MyIdleWatcher>(MAKE_NAME(MyIdleWatcher));
                RegisterObject<MyAsyncWatcher>(MAKE_NAME(MyAsyncWatcher));
            }
            
            MyWatcherManager(MyWatcherManager& r) = delete;
            MyWatcherManager& operator = (MyWatcherManager& r) = delete;
            
            /**
             *  @brief 析构函数
             */
            ~MyWatcherManager() {
                std::lock_guard<std::mutex> guard(mutex_);
                for (auto it = watcher_list_.begin(); it != watcher_list_.end(); ++it) {
                    MyWatcher::destroy(*it);
                }
                watcher_list_.clear();
            }
            
            /**
             *  @brief 获取一个新的watcher
             *
             *  @param pred 需要执行的函数
             *
             *  @return 生成的指针
             */
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MyIOWatcher>::value, T*>::type create(Pred&& pred, int32_t fd, int32_t revents) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MyIOWatcher), pred));
                obj->set(fd, revents);
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MyTimerWatcher>::value, T*>::type create(Pred&& pred, ev_tstamp after, ev_tstamp repeat) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MyTimerWatcher), pred));
                obj->set(after, repeat);
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MySignalWatcher>::value, T*>::type create(Pred&& pred, int32_t signal) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MySignalWatcher), pred));
                obj->set(signal);
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MyStatWatcher>::value, T*>::type create(Pred&& pred, const std::string& path, ev_tstamp interval) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MyStatWatcher), pred));
                obj->set(path, interval);
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MyIdleWatcher>::value, T*>::type create(Pred&& pred) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MyIdleWatcher), pred));
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            template<typename T, typename Pred>
            typename std::enable_if<std::is_same<T, MyAsyncWatcher>::value, T*>::type create(Pred&& pred) {
                auto obj = static_cast<T*>(factory_.MakeObject(MAKE_NAME(MyAsyncWatcher), pred));
                add(static_cast<MyWatcher*>(obj));
                return obj;
            }
            
            /**
             *  @brief 删除一个watcher
             *
             *  @param watcher 监听的事件
             */
            void destroy(MyWatcher* watcher) {
                remove(watcher);
                MyWatcher::destroy(watcher);
            }
        protected:
            //增加到watcher list
            void add(MyWatcher* watcher) {
                std::lock_guard<std::mutex> guard(mutex_);
                watcher_list_.insert(watcher);
            }
            
            //从watcher list 删除
            void remove(MyWatcher* watcher) {
                std::lock_guard<std::mutex> guard(mutex_);
                watcher_list_.erase(watcher);
            }
            
            //注册Creator
            template<typename T>
            void RegisterObject(const std::string& idx) {
                factory_.RegisterCreator(idx,std::move(MyWatcher::create<T>));
            }
        private:
            MyWatcherFactory factory_;
            std::set<MyWatcher*> watcher_list_; //已经分配的watcher list
            std::mutex mutex_;
        };
    }
}

#endif /* myevwatcher_h */
