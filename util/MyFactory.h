//
//  myfactory.h
//  MF
//  factory模板类
//  Created by mingweiliu on 17/2/6.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myfactory_h
#define myfactory_h

#include <iostream>
#include <map>
#include <functional>
#include <mutex>
#define MAKE_NAME(__class) #__class

namespace MF {
    /// 工厂模板类，用于创建一个指针对象
    template <typename IdType, typename ObjectType, class... Args>
    class MyPointerFactory {
    public:
        //定义类型
        typedef std::function<ObjectType (Args...)> CreatorType; //对象创建函数
        typedef typename CreatorType::result_type result_type; //创建类型
        typedef std::map<IdType, CreatorType> RegisterMap; //已经注册的对象列表
        
        /**
         *  @brief 注册一个Creator
         *
         *  @param idx     对象索引
         *  @param creator 创建函数
         */
        void RegisterCreator(const IdType& idx, CreatorType&& creator) {
            std::lock_guard<std::mutex> guard(mutex_);
            register_map_[idx] = creator;
        }
        
        /**
         *  @brief 创建一个对象
         *
         *  @param args 对象的参数
         *
         *  @return 生成的对象
         */
        result_type MakeObject(const IdType& idx, Args&&... args) {
            std::lock_guard<std::mutex> guard(mutex_);
            
            auto it = register_map_.find(idx);
            if (it == register_map_.end()) {
                return nullptr;
            } else {
                return (it->second)(args...);
            }
        }
    protected:
    private:
        std::mutex mutex_;
        RegisterMap register_map_; //已经注册的对象列表
    };
}

#endif /* myfactory_h */
