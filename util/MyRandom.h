//
//  myrandom.h
//  MyFramewrok
//  随机数生成器封装，每个线程独占一个seed
//  Created by mingweiliu on 2017/2/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myrandom_h
#define myrandom_h

#include <random>
#include "MyCommon.h"

namespace MF {
    //32位的随机数引擎
    template<typename T
    , class Engine
    , class Dist>
    class MyRandomEngine {
    public:
        MyRandomEngine() {
            engine_.seed(device_()); //设置seed
        }
        /**
         *  @brief 生成一个随机数 [min, max]
         *
         *  @param min 最小值
         *  @param max 最大是
         *
         *  @return 随机数
         */
        typename std::enable_if<std::is_integral<T>::value, T>::type
        rand(T min, T max) {
            T num_min = std::numeric_limits<T>::min();
            T num_max = std::numeric_limits<T>::max();
            if (min < num_min || max > num_max) { //需要生成的范围超过了可生成的范围
                throw MyException(std::string("rand overflow, min: ")
                                  + MyCommon::tostr(min)
                                  + ", max: "
                                  + MyCommon::tostr(max), 0);
            }
            
            //生成随机数
            Dist d(min, max);
            return d(engine_);
        }
        
        /**
         *  @brief 重载()
         *
         *  @param min 最小值
         *  @param max 最大值
         *
         *  @return 随机数
         */
        typename std::enable_if<std::is_integral<T>::value, T>::type
        operator () (T min, T max) {
            return rand(min, max);
        }
    private:
        std::random_device device_; //device
        Engine engine_; //engine
    };
    
    class MyRandom; //先申明
    //随机数生成器的实现
    template<typename T
    , class Engine
    , class Dist = std::uniform_int_distribution<T>>
    class MyRandomImpl {
    public:
        typedef MyRandomEngine<T, Engine, Dist> reference_type;
    private:
        friend class MyRandom;
        /**
         *  @brief 获取rng对象
         *
         *  @return rng对象
         */
        static MyRandomEngine<T, Engine, Dist>* rng() {
            return &rng_;
        }
    private:
        static thread_local MyRandomEngine<T, Engine, Dist> rng_; //随机数引擎
    };
    //定义rng
    template<typename T , class Engine , class Dist>
    thread_local MyRandomEngine<T, Engine, Dist> MyRandomImpl<T, Engine, Dist>::rng_;
    
    typedef MyRandomImpl<uint32_t, std::mt19937> MyRandomImpl32;
    typedef MyRandomImpl<uint64_t, std::mt19937_64> MyRandomImpl64;
    
    /// 随机数生成器封装类
    class MyRandom {
    public:
        /**
         *  @brief 随机获取uint32
         *
         *  @return 随机数
         */
        static uint32_t rand32() {
            return (*MyRandomImpl32::rng())(std::numeric_limits<uint32_t>::min()
                                         , std::numeric_limits<uint32_t>::min());
        }
        static uint32_t rand32(uint32_t max) {
            return (*MyRandomImpl32::rng())(std::numeric_limits<uint32_t>::min()
                                            , max);
        }
        static uint32_t rand32(uint32_t min, uint32_t max) {
            return (*MyRandomImpl32::rng())(min, max);
        }
        
        /**
         *  @brief 随机获取uint64
         *
         *  @return 随机数
         */
        static uint64_t rand64() {
            return (*MyRandomImpl64::rng())(std::numeric_limits<uint64_t>::min()
                                            , std::numeric_limits<uint64_t>::min());
        }
        static uint64_t rand64(uint32_t max) {
            return (*MyRandomImpl64::rng())(std::numeric_limits<uint64_t>::min()
                                            , max);
        }
        static uint64_t rand64(uint32_t min, uint32_t max) {
            return (*MyRandomImpl64::rng())(min, max);
        }
    protected:
    private:
    };
}

#endif /* myrandom_h */
