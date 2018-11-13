//
//  mytimeoutmap.h
//  MyFramewrok
//  将map按照1秒的间隔，分割成小部分，然后将这些小部分保存在deque中
//  根据当前时间与超时时长做比较，确定需要删除几个小块
//  当某个元素被更新时，将该元素放入到最后的分片中
//  目前只能包含智能指针, 如果需要包含其他指针的话，那么必须自己实现Wrapper
//
//  Created by mingweiliu on 2017/2/20.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mytimeoutmap_h
#define mytimeoutmap_h

#include <iostream>
#include <map>
#include <set>
#include <deque>
#include <mutex>
#include "MyTimeProvider.h"
#include "MyRandom.h"
#include <glog/logging.h>

namespace MF {
    //Key: key类型
    //ValueT: 包含的Value类型
    //Destructor: 析构器
    template<typename Key, typename ValueT, template<class> class Destructor>
    class MyTimeoutMap {
    private:
        template<typename T>
        struct ValueWrapper {
            uint32_t ts_; //时间戳
            T v_; //值
        };
        template<typename T>
        struct KeyWrapper {
            uint32_t ts_;
            std::set<T> keys_;
        };
        
        typedef ValueT Value;

    public:
        /**
         *  @brief 获取一个Value, 如果没有这个key则
         *
         *  @param k key
         *
         *  @return value
         */
        Value find(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = values_.find(k);
            Value ptr = nullptr;
            if (it != values_.end()) {
                ptr = it->second.v_;
            }
            
            return ptr;
        }
        
        /**
         *  @brief 获取第一个元素
         *
         *  @return empty时 为nullptr
         */
        Value movefirst() {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = values_.begin();
            Value ptr = nullptr;
            if (it != values_.end()) {
                ptr = it->second.v_;
                erase_with_nolock(it->first);
            }
            
            return ptr;
        }
        
        /**
         *  @brief 随机获取一个元素
         *
         *  @return 获取到的数据
         */
        Value random() {
            std::lock_guard<std::mutex> guard(mutex_);
            if (values_.empty()) {
                return nullptr;
            }
            auto idx = MyRandom::rand32(0, MyCast::to<uint32_t>(keys_.size() - 1));
            auto it = values_.begin();
            std::advance(it, idx);
            return it->second.v_;
        }
        
        /**
         *  @brief 写入一个数据
         *
         *  @param k key
         *  @param v value
         */
        void insert(const Key& k, Value v) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取当前时间
            uint32_t now = MyTimeProvider::now();
            
            //2. 检查是否需要更新KeyWrapper
            KeyWrapper<Key>* wrapper = nullptr;
            wrapper = &(keys_[now]);
            wrapper->ts_ = now;
            wrapper->keys_.insert(k);
            
            //3. 更新到map
            values_[k].ts_ = now;
            values_[k].v_ = v;
        }
        
        /**
         *  @brief 更新一个key的激活时间
         *
         *  @param k key
         */
        void update(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 更新values中存储的key的更新时间
            auto now = MyTimeProvider::now();
            ValueWrapper<Value>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            vp->ts_ = now;
            
            //3. 删除原来的keyset
            KeyWrapper<Key>* oldkp = &(keys_[oldts]);
            oldkp->keys_.erase(k);
            
            //4. 更新KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[now]);
            kp->keys_.insert(k);
            kp->ts_ = now;
        }
        
        /**
         *  @brief  更新一个key的值，并且更新激活时间
         *
         *  @param k key
         *  @param v value
         */
        void update(const Key& k, Value v) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 更新values中存储的key的更新时间
            auto now = MyTimeProvider::now();
            ValueWrapper<Value>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            vp->ts_ = now;
            vp->v_ = v;
            
            //3, 删除原来的keyset
            KeyWrapper<Key>* oldkp = &(keys_[oldts]);
            oldkp->keys_.erase(k);
            
            //4. 更新KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[now]);
            kp->keys_.insert(k);
            kp->ts_ = now;

        }
        
        /**
         *  @brief 删除一个key, 返回被删除的指针
         *
         *  @param k key
         */
        Value erase(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return nullptr;
            }
            
            //2. 删除ValueWrapper
            ValueWrapper<Value>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            values_.erase(k);
            
            //3. 删除KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[oldts]);
            kp->keys_.erase(k);
            
            //4. 检查是否要删除整个KeyWrapper
            if (kp->keys_.empty()) {
                keys_.erase(oldts);
            }
            
            return vp->v_;
        }
        
        /**
         *  @brief 清理所有的key
         */
        void clear() {
            std::lock_guard<std::mutex> guard(mutex_);
            keys_.clear();
            values_.clear();
        }
        
        typedef std::function<void(const Key& k, Value v)> TimeoutFunc;
        /**
         *  @brief 执行超时操作
         */
        void DoTimeout(TimeoutFunc&& pred) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 获取应当超时的时间
            uint32_t ts = MyTimeProvider::now() - timeout_;
            
            //2. 获取应当超时的所有key
            auto endit = keys_.lower_bound(ts);
            
            //3. 删除超时key
            for (auto it = keys_.begin(); it != endit; ) {
                //删除所有的key
                for (auto k = it->second.keys_.begin(); k != it->second.keys_.end(); ++k) {
                    LOG(INFO) << "timeout key: " << *k << ", ts:" << it->first << std::endl;
                    auto v = values_[*k].v_;
                    pred(*k, v); //执行超时函数
                    destructor_.destroy(v);
                    values_.erase(*k);
                }
                it = keys_.erase(it);
            }
        }
        
        void DoTimeout() {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 获取应当超时的时间
            uint32_t ts = MyTimeProvider::now() - timeout_;
            
            //2. 获取应当超时的所有key
            auto endit = keys_.lower_bound(ts);
            
            //3. 删除超时key
            for (auto it = keys_.begin(); it != endit; ) {
                //删除所有的key
                for (auto k = it->second.keys_.begin(); k != it->second.keys_.end(); ++k) {
                    LOG(INFO) << "timeout key: " << *k << ", ts:" << it->first << std::endl;
                    auto v = values_[*k].v_;
                    destructor_.destroy(v);
                    values_.erase(*k);
                }
                it = keys_.erase(it);
            }
        }
        
        /**
         *  @brief 设置超时时间
         *
         *  @param timeout 超时时长(秒)
         */
        void set_timeout(uint32_t timeout) {
            timeout_ = timeout;
        }
        
        uint32_t timeout() const {
            return timeout_;
        }
        
        //返回是否为空
        bool empty() const {
            return values_.empty();
        }
        
        //返回map的元素个数
        uint32_t size() const {
            return values_.size();
        }
        
    protected:
        /**
         *  @brief 不锁定删除
         *
         *  @param k key
         */
        void erase_with_nolock(const Key& k) {
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 删除ValueWrapper
            ValueWrapper<ValueT>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            values_.erase(k);
            
            //3. 删除KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[oldts]);
            kp->keys_.erase(k);
            
            //4. 检查是否要删除整个KeyWrapper
            if (kp->keys_.empty()) {
                keys_.erase(oldts);
            }
        }
    private:
        std::map<Key, ValueWrapper<Value>> values_;
        std::map<uint32_t, KeyWrapper<Key>> keys_; //被分割成1秒的key的小分片
        uint32_t timeout_; //超时时长(秒)
        std::mutex mutex_; //锁
        Destructor<Value> destructor_; //析构器
    };
    
    template<typename T>
    struct SharedPtrDestructor {
        void destroy(T v) {
            //什么都不做
        }
    };
    
    //共享指针的map
    template<typename Key, typename ValueT>
    class MySharedTimeoutMap : public MyTimeoutMap<Key, ValueT, SharedPtrDestructor> {
        
    };
    
    //unique_ptr的超时map
    template<typename Key, typename ValueT>
    class MyUniqueTimeoutMap {
    private:
        template<typename T>
        struct ValueWrapper {
            uint32_t ts_; //时间戳
            std::unique_ptr<T> v_; //值
        };
        template<typename T>
        struct KeyWrapper {
            uint32_t ts_;
            std::set<T> keys_;
        };
        
        typedef std::unique_ptr<ValueT> Value;
        typedef ValueT* value_pointer;
        
    public:
        /**
         *  @brief 获取一个Value, 如果没有这个key则
         *
         *  @param k key
         *
         *  @return value
         */
        value_pointer find(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = values_.find(k);
            value_pointer ptr = nullptr;
            if (it != values_.end()) {
                ptr = it->second.v_.get();
            }
            
            return ptr;
        }
        
        /**
         *  @brief 将一个unique_ptr move出来
         *
         *  @param k key
         *
         *  @return value
         */
        Value moveout(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = values_.find(k);
            Value ptr = nullptr;
            if (it != values_.end()) {
                ptr = std::move(it->second.v_);
                erase_with_nolock(k);
            }
            
            return ptr;
        }
        
        /**
         *  @brief 将第一个unique_ptr move出来， 用这个函数来遍历
         *
         *  @return 返回的指针
         */
        Value movefirst() {
            std::lock_guard<std::mutex> guard(mutex_);
            auto it = values_.begin();
            Value ptr = nullptr;
            if (it != values_.end()) {
                ptr = std::move(it->second.v_);
                erase_with_nolock(it->first);
            }
            
            return ptr;
        }
        
        /**
         *  @brief 随机获取一个元素
         *
         *  @return 获取到的数据
         */
        value_pointer random() {
            std::lock_guard<std::mutex> guard(mutex_);
            if (values_.empty()) {
                return nullptr;
            }
            auto idx = MyRandom::rand32(0, MyCast::to<uint32_t>(keys_.size() - 1));
            auto it = values_.begin();
            std::advance(it, idx);
            return it->second.v_.get();
        }
        
        /**
         *  @brief 写入一个数据
         *
         *  @param k key
         *  @param v value
         */
        void insert(const Key& k, Value v) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取当前时间
            uint32_t now = MyTimeProvider::now();
            
            //2. 检查是否需要更新KeyWrapper
            KeyWrapper<Key>* KeyWrapper = nullptr;
            KeyWrapper = &(keys_[now]);
            KeyWrapper->ts_ = now;
            KeyWrapper->keys_.insert(k);
            
            //3. 更新到map
            values_[k].ts_ = now;
            values_[k].v_ = std::move(v);
        }
        
        /**
         *  @brief 更新一个key的激活时间
         *
         *  @param k key
         */
        void update(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 更新values中存储的key的更新时间
            auto now = MyTimeProvider::now();
            ValueWrapper<Value>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            vp->ts_ = now;
            
            //3. 删除原来的keyset
            KeyWrapper<Key>* oldkp = &(keys_[oldts]);
            oldkp->keys_.erase(k);
            
            //4. 更新KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[now]);
            kp->keys_.insert(k);
            kp->ts_ = now;
        }
        
        /**
         *  @brief  更新一个key的值，并且更新激活时间
         *
         *  @param k key
         *  @param v value
         */
        void update(const Key& k, Value v) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 更新values中存储的key的更新时间
            auto now = MyTimeProvider::now();
            ValueWrapper<Value>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            vp->ts_ = now;
            vp->v_ = std::move(v);
            
            //3, 删除原来的keyset
            KeyWrapper<Key>* oldkp = &(keys_[oldts]);
            oldkp->keys_.erase(k);
            
            //4. 更新KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[now]);
            kp->keys_.insert(k);
            kp->ts_ = now;
            
        }
        
        /**
         *  @brief 删除一个key
         *
         *  @param k key
         */
        void erase(const Key& k) {
            std::lock_guard<std::mutex> guard(mutex_);
            erase_with_nolock(k);
        }
        
        /**
         *  @brief 清理所有的key
         */
        void clear() {
            std::lock_guard<std::mutex> guard(mutex_);
            keys_.clear();
            values_.clear();
        }
        
        typedef std::function<void(const Key& k, Value v)> TimeoutFunc;
        /**
         *  @brief 执行超时操作
         */
        void DoTimeout(TimeoutFunc&& pred) {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 获取应当超时的时间
            uint32_t ts = MyTimeProvider::now() - timeout_;
            
            //2. 获取应当超时的所有key
            auto endit = keys_.lower_bound(ts);
            
            //3. 删除超时key
            for (auto it = keys_.begin(); it != endit; ) {
                //删除所有的key
                for (auto k = it->second.keys_.begin(); k != it->second.keys_.end(); ++k) {
                    LOG(INFO) << "timeout key: " << *k << ", ts:" << it->first << ", timeout: " << timeout_ << std::endl;
                    pred(*k, std::move(values_[*k].v_)); //执行超时函数
                    values_.erase(*k);
                }
                it = keys_.erase(it);
            }
        }
        
        void DoTimeout() {
            std::lock_guard<std::mutex> guard(mutex_);
            //1. 获取应当超时的时间
            uint32_t ts = MyTimeProvider::now() - timeout_;
            
            //2. 获取应当超时的所有key
            auto endit = keys_.lower_bound(ts);
            
            //3. 删除超时key
            for (auto it = keys_.begin(); it != endit; ) {
                //删除所有的key
                for (auto k = it->second.keys_.begin(); k != it->second.keys_.end(); ++k) {
                    LOG(INFO) << "timeout key: " << *k << ", ts:" << it->first << std::endl;
                    values_.erase(*k);
                }
                it = keys_.erase(it);
            }
        }
        
        /**
         *  @brief 设置超时时间
         *
         *  @param timeout 超时时长(秒)
         */
        void set_timeout(uint32_t timeout) {
            timeout_ = timeout;
        }
        
        //返回是否为空
        bool empty() const {
            return values_.empty();
        }
        
        //返回map的元素个数
        uint32_t size() const {
            return values_.size();
        }
        
    protected:
        
        /**
         *  @brief 不锁定删除
         *
         *  @param k key
         */
        void erase_with_nolock(const Key& k) {
            //1. 先获取该key上次存储的时间
            if (values_.find(k) == values_.end()) {
                return;
            }
            
            //2. 删除ValueWrapper
            ValueWrapper<ValueT>* vp = &(values_[k]);
            auto oldts = vp->ts_;
            values_.erase(k);
            
            //3. 删除KeyKeyWrapper
            KeyWrapper<Key>* kp = &(keys_[oldts]);
            kp->keys_.erase(k);
            
            //4. 检查是否要删除整个KeyWrapper
            if (kp->keys_.empty()) {
                keys_.erase(oldts);
            }
        }
    private:
        std::map<Key, ValueWrapper<ValueT>> values_;
        std::map<uint32_t, KeyWrapper<Key>> keys_; //被分割成1秒的key的小分片
        uint32_t timeout_; //超时时长(秒)
        std::mutex mutex_; //锁
    };
}

#endif /* mytimeoutmap_h */
