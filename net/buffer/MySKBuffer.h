//
//  myskbuffer.h
//  MF
//  SKBuffer，使用一整段内存来标示哪些可读，哪些可写
//  Created by mingweiliu on 17/2/8.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myskbuffer_h
#define myskbuffer_h

#include <iostream>
#include <cstring>
#include <cassert>

namespace MF {
    
    namespace Buffer {
        /// Head + Data
        //考虑到尽量少的数据拷贝，所以不支持多线程操作
        //当用在socket上时，需要在buffer上层做好线程同步
        //当没有可写空间时，会先将已经使用的内存向前移动
        //如果移动完之后仍然没有足够的空间，buffer自动拓展
        class MySKBuffer {
        public:
            
            /**
             *  @brief 构造函数
             */
            MySKBuffer(uint32_t capacity) {
                //1. 初始化head
                head_ = InitHead(capacity);
                
                //2. 准备data
                data_ = reinterpret_cast<void*>(reinterpret_cast<char*>(head_) + sizeof(SKBufferHead));
            }
            
            /**
             *  @brief 析构函数
             */
            ~MySKBuffer() {
                std::free(head_);
            }
            
            //////////////
            
            /**
             *  @brief 获取可读数据的长度
             *
             *  @return 可读数据的长度
             */
            uint32_t getReadableLength() const {
                assert(head_->writeable_ >= head_->readable_);
                return head_->writeable_ - head_->readable_;
            }
            
            /**
             *  @brief 获取可写的长度
             *
             *  @return 可写buffer的长度
             */
            uint32_t getWriteableLength() const {
                assert(head_->capacity_ >= getReadableLength());
                return head_->capacity_ - getReadableLength();
            }
            
            /**
             *  @brief 获取可读指针的位置
             *
             *  @param length 需要读取的长度，返回实际数据的长度
             *
             *  @return 可读指针
             */
            char* readable(uint32_t* length) {
                //1. 检查是否有可读的数据
                auto buf_len = getReadableLength();
                if (buf_len == 0) { //没有数据可读取，则返回nullptr
                    *length = 0;
                    return nullptr;
                }
                
                //2. 获取可读指针
                if (*length > buf_len) { //如果需要读取的长度超过了buffer中数据的长度，那么就修改可读字节的长度
                    *length = buf_len;
                }
                return static_cast<char*>(data_) + head_->readable_;
            }
            
            /**
             *  @brief 移动可读指针的位置
             *
             *  @param length 需要移动的长度
             */
            void moveReadable(uint32_t length) {
                head_->readable_ += length;
            }
            
            /**
             *  @brief 获取可读指针
             *
             *  @param length 需要读取的长度，如果数据长度少于需要读取的长度，则返回实际的数据长度
             *  @return 可读指针
             */
            char* getReadableAndMove(uint32_t *length) const {
                //1. 检查是否有可读的数据
                auto buf_len = getReadableLength();
                if (buf_len == 0) { //没有数据可读取，则返回nullptr
                    *length = 0;
                    return nullptr;
                }
                
                //2. 获取可读指针
                auto r = static_cast<char*>(data_) + head_->readable_;
                
                //3. 移动readable指针
                if (*length > buf_len) { //如果需要读取的长度超过了buffer中数据的长度，那么就修改可读字节的长度
                    *length = buf_len;
                }
                head_->readable_ += *length;
                
                return r;
            }
            
            /**
             *  @brief 获取可写指针
             *
             *  @param length 需要写入的长度
             *
             *  @return 可写指针
             */
            char* writeable(uint32_t length) {
                //1. 检查readable和writeable是否相同，如果相同，说明read已经追上write，那么将writeable重置为0
                if (head_->readable_ == head_->writeable_) {
                    head_->readable_ = 0;
                    head_->writeable_ = 0;
                }
                //2. 检查可用空间是否足够
                if (head_->writeable_ + length > head_->capacity_) { //空间不够无法追加
                    if (getWriteableLength() >= length) { //如果总的可用空间够了，那么就重新整理当前buffer
                        sort();
                    } else {
                        incr();
                    }
                }
                
                //2. 返回可写指针
                //            auto writeable = static_cast<char*>(static_cast<char*>(data_) + head_->writeable_);
                
                //3. 移动writeable
                //            head_->writeable_ += length; //不移动writeable, 而是提供接口让调用者来移动
                
                return static_cast<char*>(static_cast<char*>(data_) + head_->writeable_);
            }
            
            /**
             *  @brief 获取可写指针, 并且自动移动writeable
             *
             *  @param length 需要写入的长度
             *
             *  @return 可写指针
             */
            char* getWriteableAndMove(uint32_t length) {
                //1. 检查readable和writeable是否相同，如果相同，说明read已经追上write，那么将writeable重置为0
                if (head_->readable_ == head_->writeable_) {
                    head_->readable_ = 0;
                    head_->writeable_ = 0;
                }
                //2. 检查可用空间是否足够
                if (head_->writeable_ + length > head_->capacity_) { //空间不够无法追加
                    if (getWriteableLength() >= length) { //如果总的可用空间够了，那么就重新整理当前buffer
                        sort();
                    } else {
                        incr();
                    }
                }
                
                //2. 返回可写指针
                auto writeable = static_cast<char*>(static_cast<char*>(data_) + head_->writeable_);
                
                //3. 移动writeable
                moveWriteable(length);
                
                return writeable;
            }
            
            /**
             *  @brief 移动write able, 在调用writeable之后在调用move_writeable, 否则可能会导致内存被改写
             *
             *  @param length 需要移动的长度
             */
            void moveWriteable(uint32_t length) {
                head_->writeable_ += length;
            }
            
            /**
             *  @brief 获取容量
             *
             *  @return 容量
             */
            uint32_t capacity() const {
                return head_->capacity_;
            }
            
            //for test
            uint32_t readablev() const {
                return head_->readable_;
            }
            
            uint32_t writeablev() const {
                return head_->writeable_;
            }
            
            void reset() {
                head_->readable_ = 0;
                head_->writeable_ = 0;
                std::memset(data_, 0, head_->capacity_);
            }
            
        protected:
            
            /**
             *  @brief 重新排序数据内容
             */
            void sort() {
                //1. 判断readable的位置
                if (head_->readable_ == 0) { //如果当前已经在最前了，那么不移动
                    return;
                }
                
                //2. 移动内存
                void* src = static_cast<void*>(static_cast<char*>(data_) + head_->readable_);
                
                uint32_t len = getReadableLength();
                std::memmove(data_, src, len);
                
                //3. 重新设置readable和writeable
                head_->readable_ = 0;
                head_->writeable_ = head_->readable_ + len;
                
                //4. 重置可写内存
                std::memset(static_cast<char*>(data_) + head_->writeable_, 0x00, head_->capacity_ - head_->writeable_);
            }
            
            /**
             *  @brief 扩容空间
             */
            void incr() {
                //1. 生成new head
                auto newhead = InitHead(head_->capacity_ + min_capacity_);
                
                //2. 拷贝数据
                std::memcpy(reinterpret_cast<char*>(newhead) + sizeof(uint32_t), reinterpret_cast<char*>(head_) + sizeof(uint32_t), head_->capacity_); //不拷贝头4个字节
                
                //3. 释放旧的head，并且设置新的head和data
                std::free(head_);
                head_ = newhead;
                data_ = reinterpret_cast<void*>(reinterpret_cast<char*>(head_) + sizeof(SKBufferHead));
                
                //4. 重新排序newhead
                sort();
            }
            
        private:
            
            //头结点，用于记录一些信息
            struct SKBufferHead {
                uint32_t capacity_; //buffer容量
                uint32_t readable_; //可读位置标识
                uint32_t writeable_; //可写位置标识
            }__attribute__((packed));
            
            /**
             初始化head节点
             
             - returns: 无
             */
            SKBufferHead* InitHead(uint32_t capacity) {
                //1. 分配内存
                min_capacity_ = capacity;
                void* buf = std::malloc(min_capacity_ + sizeof(SKBufferHead));
                std::memset(buf, 0x00, min_capacity_);
                
                //2. 操作head
                SKBufferHead* head = (SKBufferHead*)buf;
                head->capacity_ = min_capacity_;
                head->readable_ = 0;
                head->writeable_ = 0;
                
                return head;
            }
            
            uint32_t min_capacity_; //最小容量
            
            SKBufferHead* head_; //头结点
            void* data_; //数据节点
        };
    }
}

#endif /* myskbuffer_h */
