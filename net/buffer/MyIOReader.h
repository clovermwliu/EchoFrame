//
//  myioreader.h
//  MF
//
//  Created by mingweiliu on 2017/2/14.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myioreader_h
#define myioreader_h

#include "util/MyException.h"
#include "util/MyCommon.h"

namespace  MF {
    namespace Buffer {
        template<typename> class MyReader;
        
        /// 基本类型的Reader
        template<typename T>
        class MyReader {
        public:
            /**
             *  @brief 读一个只
             *
             *  @param buf 码流
             *
             *  @return 读取到的值
             */
            static T&& read(void* buf) {
                T v;
                std::memcpy(reinterpret_cast<void*>(&v), buf, sizeof(T));
                return std::move(v);
            }
        };
        
        template<>
        class MyReader<char*> {
        public:
            /**
             *  @brief 读一个只
             *
             *  @param buf 码流
             *
             */
            static void read(void* buf, char* v, uint32_t length) {
                std::memcpy(reinterpret_cast<void*>(v), buf, length);
            }
        };
        
        template<>
        class MyReader<void*> {
        public:
            /**
             *  @brief 读一个只
             *
             *  @param buf 码流
             *
             */
            static void read(void* buf, void* v, uint32_t length) {
                std::memcpy(reinterpret_cast<void*>(v), buf, length);
            }
        };
        
        template<>
        class MyReader<std::string> {
        public:
            /**
             *  @brief 读一个只
             *
             *  @param buf 码流
             *
             *  @return 读取到的值
             */
            static std::string&& read(void* buf, uint32_t length) {
                std::string v;
                v.assign(reinterpret_cast<char*>(buf), length);
                return std::move(v);
            }
        };
    }
}

#endif /* myioreader_h */
