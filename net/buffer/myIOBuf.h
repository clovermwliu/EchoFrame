//
//  myiobuf.h
//  MF
//
//  Created by mingweiliu on 2017/2/14.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myiobuf_h
#define myiobuf_h

#include "net/buffer/MySKBuffer.h"
#include "net/buffer/MyIOWriter.h"
#include "net/buffer/MyIOReader.h"

namespace MF {
    namespace Buffer {
        /// 基于SKBuffer的IOBuf
        /// SKBuffer是一个基于内存的，自动增长的buffer
        class MyIOBuf {
        public: //不允许手动构造
            /**
             *  @brief 构造函数
             */
            MyIOBuf(uint32_t capacity) {
                buffer_ = new MySKBuffer(capacity);
            }
            
            /**
             *  @brief 析构函数
             */
            ~MyIOBuf() {
                delete buffer_;
            }
            
            /**
             *  @brief 包装一个Skbuffer
             *
             *  @param buffer 数据
             */
            void wrapper(MySKBuffer* buffer) {
                buffer_ = buffer;
            }

            /**
             *  @brief 构造一个IObuf对象
             *
             *  @param capacity 初始容量， 默认1K
             *
             *  @return IObuf对象
             */
            static std::unique_ptr<MyIOBuf> create(uint32_t capacity = 1024) {
                std::unique_ptr<MyIOBuf> iobuf(new MyIOBuf(capacity));
                return iobuf;
            }

            /**
             *  @brief 交换两个IOBuf
             *
             *  @param r 右值
             */
            void swap(MyIOBuf& r) {
                std::swap(buffer_, r.buffer_);
            }

            /**
             *  @brief 获取可读数据的长度
             *
             *  @return 长度
             */
            uint32_t getReadableLength() const {
                return buffer_->getReadableLength();
            }
            
            /**
             *  @brief 获取可读指针
             *
             *  @return 可读指针
             */
            void* readable() {
                uint32_t len = buffer_->getReadableLength();
                return buffer_->readable(&len);
            }

            /**
             * 移动可读指针
             * @param len 长度
             */
            void moveReadable(uint32_t len) {
                buffer_->moveReadable(len);
            }

        public:
            
            //基本类型
#define BASIC_TYPE(T) typename std::enable_if<!std::is_pointer<T>::value && std::is_object<T>::value && !std::is_class<T>::value, T>::type
            
            //指针类型
#define POINTER_TYPE(T) typename std::enable_if<std::is_pointer<T>::value, T>::type
            
            //类类型
#define CLASS_TYPE(T) typename std::enable_if<std::is_class<T>::value, T>::type
            
            ////////////////////////////////////////////////
            
            /**
             *  @brief 写入一个非指针字段(基本类型)
             *
             *  @param v 非指针字段
             *
             */
            template<typename T> void write (const BASIC_TYPE(T) &v);
            
            /**
             *  @brief 写入一个非指针字段(类类型)
             *
             *  @param v 非指针字段
             *
             */
            template<typename T> void write (const CLASS_TYPE(T) &v);
            
            /**
             *  @brief 写入一个指针字段
             *
             *  @param v 需要写入的指针
             *  @param length 指着的长度
             *
             
             */
            template<typename T> void write (POINTER_TYPE(T) v, uint32_t length);
            
            /**
             *  @brief 预分配一部分内存
             *
             *  @param length 预分配的长度
             *
             *  @return 返回的指针
             */
            void* reserve(uint32_t length) {
                return buffer_->getWriteableAndMove(length); //直接获取可写指针返回出去
            }
            
            ////////////////////////////////////////////////
            //读取
            /**
             *  @brief 读取一个基本类型
             *  @return 读取到的值
             */
            template<typename T> BASIC_TYPE(T) read();
            
            /**
             *  @brief 读取数据
             *
             *  @param v buffer
             *
             */
            template<typename T> void read(POINTER_TYPE(T) v, uint32_t length);
            
            /**
             *  @brief 读取一个类类型
             *
             *  @param length 需要读取的长度
             *
             *  @return 读取到的值
             */
            template<typename T> CLASS_TYPE(T) read(uint32_t length);


        public:
            MyIOBuf(const MyIOBuf& r) = delete;
            MyIOBuf& operator = (const MyIOBuf& r) = delete; //不能赋值， SKBuffer不能多线程操作
        private:
            MySKBuffer* buffer_;
        };
        
        template<typename T> void MyIOBuf::write (const BASIC_TYPE(T) &v) {
            //1. 先获取数据的长度
            uint32_t len = sizeof(T);
            void* buf = buffer_->getWriteableAndMove(len);
            if (!buf) {
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 写入数据
            MyWriter<T>::write(buf, v);
        }
        
        template<typename T> void MyIOBuf::write (const CLASS_TYPE(T) &v) {
            //1. 先获取数据的长度
            uint32_t len = MyWriter<T>::getLength(v);
            if (!len) { //长度为0，不需要改字段
            }
            void* buf = buffer_->getWriteableAndMove(len);
            if (!buf) {
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 写入数据
            MyWriter<T>::write(buf, v);
        }
        
        template<typename T> void MyIOBuf::write(POINTER_TYPE(T) v, uint32_t length) {
            
            //1. 先获取数据的长度
            void* buf = buffer_->getWriteableAndMove(length);
            if (!buf) {
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 写入数据
            MyWriter<T>::write(buf, v, length);
        }
        
        template<typename T> BASIC_TYPE(T) MyIOBuf::read() {
            //1. 先获取类型长度
            uint32_t length = sizeof(T);
            void* buf = buffer_->getReadableAndMove(&length);
            if (!buf || length < sizeof(T)) { //长度不够
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 读取数据
            return MyReader<T>::read(buf);
        }
        
        template<typename T> void MyIOBuf::read(POINTER_TYPE(T) v, uint32_t length) {
            //1. 先获取类型长度
            uint32_t tmp = length;
            void* buf = buffer_->getReadableAndMove(&tmp);
            if (!buf || tmp < length) { //长度不够
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 读取数据. IOBuf不分配内存
            return MyReader<T>::read(buf, v, length);
        }
        
        template<typename T> CLASS_TYPE(T) MyIOBuf::read(uint32_t length) {
            //1. 先获取类型长度
            uint32_t tmp = length;
            void* buf = buffer_->getReadableAndMove(&tmp);
            if (!buf || tmp < length) { //长度不够
                throw MyException(std::string("[") + std::string(__FILE__) + ":" + MyCommon::tostr(__LINE__) + "] SkBuffer overflow", 0);
            }
            
            //2. 读取数据. IOBuf不分配内存
            return MyReader<T>::read(buf, length);
        }
    }
}

#endif /* myiobuf_h */
