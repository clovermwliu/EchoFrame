//
//  mysingleton.h
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mysingleton_h
#define mysingleton_h

#include <stdio.h>
#include <cstdlib>
#include <stdexcept>
#include <mutex>

namespace MF {
    /// 使用operator new创建内存
    template <typename T>
    class MyCreateNew {
    public:
        
        /**
         *  @brief 创建一个对象
         *
         *  @return 创建的对象
         */
        static T* create() {
            return new T();
        }
        
        /**
         *  @brief 删除一个对象
         *
         *  @param t 对象
         */
        static void destroy(T* t) {
            delete t;
        }
    };
    
    /// 使用malloc构造
    template <typename T>
    class MyCreateMalloc {
    public:
        /**
         *  @brief 创建一个对象
         *
         *  @return 对象
         */
        static T* create() {
            char* obj = (char*)(std::malloc(size()));
            
            return new(obj) T;
        }
        
        /**
         *  @brief 删除一个对象
         *  @notice placement new不能直接删除，需要显式调用析构函数
         *
         *  @param t 对象
         */
        static void destroy(T* t) {
            t->~T();
            std::free((void*)t); //删除分配的内存
        }
    protected:
        /**
         *  @brief 返回需要分配的内存大小
         *  @notice 使用placement new需要注意一点，所需要的内存大小至少要比实际对象大小大sizeof(int)
         *
         *  @return 需要分配的内存大小
         */
        static size_t size() {
            return sizeof(T) + sizeof(int32_t);
        }
    };
    
    /// 使用栈空间分配
    template <typename T>
    class MyCreateStatic {
    public:
        /**
         *  @brief 对象
         */
        typedef union Object {
            char t[sizeof(T)]; //存储对象
            int32_t extra; //palcement new需要的额外空间
        }Object;
        /**
         *  @brief 创建一个对象
         *
         *  @return 对象
         */
        static T* create() {
            static Object obj;
            return new(&obj) T;
        }
        
        /**
         *  @brief 删除一个对象
         *  @notice placement new不能直接删除，需要显式调用析构函数
         *
         *  @param t 对象
         */
        static void destroy(T* t) {
            t->~T();
        }
    protected:
    };
    
    /// 默认生命周期
    template <typename T>
    class MyDefaultLifeTime {
    public:
        /**
         *  @brief 对象死亡之后继续引用
         */
        static void DeadReference() {
            throw  std::logic_error("object is dead");
        }
        
        /**
         *  @brief 退出时调用析构函数
         *
         *  @param t    需要析构的指针
         *  @param func 函数
         */
        static void ScheduleDestructor(T* t, void (*func)()) {
            std::atexit(func);
        }
    };
    
    /// 永生不死的单例
    template < typename T>
    class MyPhoenixLifeTime {
    public:
        /**
         *  @brief 对象死亡之后继续引用
         */
        static void DeadReference() {
            //什么也不做,等待下面继续创建
        }
        
        /**
         *  @brief 退出时调用析构函数
         *
         *  @param t    需要析构的指针
         *  @param func 函数
         */
        static void ScheduleDestructor(T* t, void (*func)()) {
            std::atexit(func);
        }
    };
    
    /// 单例类，参考loki的设计
    template <typename T,
    template<class> class CreatePolicy = MyCreateNew,
    template<class> class LifeTimePolicy = MyDefaultLifeTime,
    typename MutexPolicy = std::mutex >
    class MySingleton {
        
    public:
        //默认的构造函数和析构函数
        MySingleton() = default;
        ~MySingleton() = default;
        
        typedef T SingletonType;
        /**
         *  @brief 获取一个单例对象
         *
         *  @return 获取到的对象
         */
        static SingletonType* GetInstance() {
            if (!instance_) { //如果是null，那么创建
                std::lock_guard<MutexPolicy> guard(mutex_);
                if (!instance_) { //第二次判断是否为空，是为了防止两个线程同时执行到获取锁这句上，产生重复创建对象的问题
                    //如果对象已经被删除了，又来调用，那么就执行lifetime的deadReference
                    //如果是默认生命周期，那么DeadReference会抛出异常
                    if (is_destroyed_) {
                        LifeTimePolicy<SingletonType>::DeadReference();
                        is_destroyed_ = false;
                    }
                    
                    //使用生命周期管理创建对象
                    instance_ = CreatePolicy<SingletonType>::create();
                    //设置对象删除函数
                    LifeTimePolicy<SingletonType>::ScheduleDestructor(instance_, MySingleton::destructor);
                }
            }
            
            return (SingletonType*)instance_;
        }
    protected:
        /**
         *  @brief 对象析构函数
         */
        static void destructor() {
            if (instance_) {
                std::lock_guard<MutexPolicy> guard(mutex_);
                CreatePolicy<SingletonType>::destroy(instance_);
                is_destroyed_ = true;
                instance_ = nullptr;
            }
        }
    private:
        //不允许拷贝和赋值
        MySingleton(MySingleton& r) = delete;
        MySingleton& operator=(MySingleton& r) = delete;
        
        static SingletonType* instance_; //实例名
        static bool is_destroyed_; //是否已经被删除
        static MutexPolicy mutex_; //锁
    };
    
    template<typename T, template<class> class CreatePolicy, template<class> class LifeTimePolicy, typename MutexPolicy>
    T* MySingleton<T, CreatePolicy, LifeTimePolicy, MutexPolicy>::instance_ = nullptr;
    
    template<typename T, template<class> class CreatePolicy, template<class> class LifeTimePolicy, typename MutexPolicy>
    bool MySingleton<T, CreatePolicy, LifeTimePolicy, MutexPolicy>::is_destroyed_ = false;
    
    template<typename T, template<class> class CreatePolicy, template<class> class LifeTimePolicy, typename MutexPolicy>
    MutexPolicy MySingleton<T, CreatePolicy, LifeTimePolicy, MutexPolicy>::mutex_;
}

#endif /* mysingleton_h */
