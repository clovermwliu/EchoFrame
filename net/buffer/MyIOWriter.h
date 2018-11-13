//
//  myiowriter.h
//  MF
//
//  Created by mingweiliu on 2017/2/14.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myiowriter_h
#define myiowriter_h

#include "util/MyException.h"
#include "util/MyCommon.h"

namespace MF {
    namespace Buffer {
        //Writer
        template<typename T> class MyWriter;
        //基本类型的Writer
        template<typename T>
        class MyWriter {
        public:
            /**
             *  @brief 写入一个数据
             *
             *  @param buf 需要写入的buffer指针
             */
            static void write(void* buf, const T& v) {
                std::memcpy(buf, reinterpret_cast<void*>(const_cast<T*>(&v)), sizeof(T));
            }
        };
        
        template<>
        class MyWriter<char*> {
        public:
            
            /**
             *  @brief 写入一个数据
             *
             *  @param buf 需要写入的buffer指针
             *  @param length 需要写入的长度
             */
            static void write(void* buf, char* v, uint32_t length) {
                std::memcpy(buf, reinterpret_cast<void*>(v), length);
            }
        };
        
        template<>
        class MyWriter<void*> {
        public:
            
            /**
             *  @brief 写入一个数据
             *
             *  @param buf 需要写入的buffer指针
             *  @param length 需要写入的长度
             */
            static void write(void* buf, void* v, uint32_t length) {
                std::memcpy(buf, v, length);
            }
        };
        
        /// string类型的Writer
        template<>
        class MyWriter<std::string> {
        public:
            
            /**
             *  @brief 写入一个数据
             *
             *  @param buf 需要写入的buffer指针
             */
            static void write(void* buf, const std::string& v) {
                std::memcpy(buf, reinterpret_cast<void*>(const_cast<char*>(v.c_str())), v.size());
            }
            
            /**
             *  @brief 获取需要写入的字段长度
             *
             *  @param v 需要写入的字段
             *
             *  @return 长度
             */
            static uint32_t getLength(const std::string &v) {
                return MyCast::to<uint32_t>(v.size());
            }
        };
        
        //void类型的Writer，不允许构造
        template<>
        class MyWriter<void> {
        private:
            void write(void* buf) = delete;
        };
    }
}

#endif /* myiowriter_h */
