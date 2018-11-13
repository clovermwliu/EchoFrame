//
//  myvariant.h
//  MyFramewrok
//
//  Created by mingweiliu on 2017/3/8.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myvariant_h
#define myvariant_h

#include "MyCommon.h"
#include <iostream>


namespace MF {
    
    //编译期间获取到类型的最大长度
    template<typename, typename...> struct MyIntegralConstant;

    //一个参数
    template<typename T>
    struct MyIntegralConstant<T> : std::integral_constant<uint32_t, sizeof(T)> {};
    
    //两个参数的定义
    template<typename T, typename... Args>
    struct MyIntegralConstant : std::integral_constant<uint32_t
    , (sizeof(T)>MyIntegralConstant<Args...>::value
       ? sizeof(T) : MyIntegralConstant<Args...>::value)> {};
    
    //编译期间决定是否包含某个类型
    //确定是否有某个类型
    template<typename T, typename... Args> struct MyContains;
    
    //查询是否有某个类型
    template<typename T, typename Head, typename... Args>
    struct MyContains<T, Head, Args...>
    : std::conditional<std::is_same<T, Head>::value
    , std::true_type, MyContains<T, Args...>>::type {};

    //不包含T类型
    template<typename T>
    struct MyContains<T> : std::false_type {};
    
    /// 泛型类型
    template<typename... Types>
    class MyVariant {
    public:
        //构造函数
        MyVariant() {
            type_hash_code_ = typeid(void).hash_code();
        }
        
        template<typename T>
        MyVariant(const typename std::enable_if<std::is_class<T>::value, T>::type & v) {
            set(v);
        }
        
        template<typename T>
        MyVariant(const typename std::enable_if<!std::is_class<T>::value, T>::type & v) {
            set(v);
        }
        
        template<typename T>
        MyVariant(typename std::enable_if<std::is_class<T>::value, T>::type && v) {
            set(v);
        }
        
        template<typename T>
        MyVariant(typename std::enable_if<!std::is_class<T>::value, T>::type && v) {
            set(v);
        }
        
        MyVariant(const MyVariant& r) {
            *this = r;
        }
        
        MyVariant(MyVariant&& r) {
            *this = std::move(r);
        }
        
        MyVariant& operator = (const MyVariant& r) {
            if (this == &r) {
                return *this;
            }
            
            if (sizeof(buffer_) != sizeof(r.buffer_)) {
                throw MyException("buffer length not suitable");
            }
            
            std::memcpy(buffer_, r.buffer_, sizeof(buffer_));
            type_hash_code_ = r.type_hash_code_;
            return *this;
        }
        
        /**
         *  @brief 检查是否有包含某个类型
         *
         *  @return true 包含 false 不包含
         */
        template<typename T> bool has() const {
            return MyContains<T, Types...>::value;
        }
        
        /**
         *  @brief 当前是否是某个类型
         *
         *  @return true 是 false 不是
         */
        template<typename T> bool is() const {
            return type_hash_code_ == typeid(typename std::decay<T>::type).hash_code();
        }
        
        /**
         *  @brief 设置一个值
         *
         *  @param v 需要设置的值
         */
        template<typename T> void set(const T& v) {
            typedef typename std::decay<T>::type value_type;
            if (!has<value_type>()) {
                throw MyException("type not supported");
            }
            (void)new (buffer_) T(v);
            type_hash_code_ = typeid(value_type).hash_code();
        }
        
        template<typename T> void set( T&& v) {
            typedef typename std::decay<T>::type value_type;
            if (!has<value_type>()) {
                throw MyException("type not supported");
            }
            (void)new (buffer_) (value_type)(std::forward<value_type>(v));
            type_hash_code_ = typeid(typename std::decay<T>::type).hash_code();
        }
        
        /**
         *  @brief 生成一个args
         *
         *  @param args 需要的参数
         */
        template<typename T, typename... Args> void make(Args... args) {
            typedef typename std::decay<T>::type value_type;
            if (!has<value_type>()) {
                throw MyException("type not supported");
            }
            (void)new (buffer_) T(args...);
            type_hash_code_ = typeid(value_type).hash_code();
        }
        
        /**
         *  @brief 获取数据值
         *
         *  @return 获取到的数据值
         */
        template<typename T> typename std::decay<T>::type as() const {
            typedef typename std::decay<T>::type value_type;
            if (!is<value_type>()) {
                throw MyException("type incorrect");
            }
            
            return *(reinterpret_cast<const value_type *>(buffer_));
        }
        
        /**
         *  @brief 获取buffer的指针
         *
         *  @return buffer的指针
         */
        template<typename T> typename std::decay<T>::type * ptr() {
            typedef typename std::decay<T>::type value_type;
            if (!is<value_type>()) {
                throw MyException("type incorrect");
            }
            
            return reinterpret_cast<value_type *>(buffer_);
        }
    private:
        //placement new需要多分配4字节
        char buffer_[MyIntegralConstant<Types...>::value + sizeof(uint32_t)];
        std::size_t type_hash_code_; //类型的hash code
    };
}

#endif /* myvariant_h */
