//
//  dcredis.h
//  DBPoolTest
//
//  Created by mingweiliu on 16/1/22.
//  Copyright (c) 2016年 mingweiliu. All rights reserved.
//

#ifndef MYFRAMEWORK2_MYREDIS_H
#define MYFRAMEWORK2_MYREDIS_H

#include "util/MyCommon.h"
#include "hiredis/hiredis.h"
using namespace std;

namespace MF {
    namespace Presistence {
        /**
         * redis配置
         */
        struct DCRedisConf {
            std::string host; //host
            uint32_t db {0}; //db number
            int16_t port {0}; //port
        };
        /**
         * Redis 操作结果帮助类
         */
        class RedisReplyHelper {
        public:
            RedisReplyHelper(redisReply* r) {
                reply = r;
            }

            ~RedisReplyHelper() {
                if(reply != nullptr){
                    freeReplyObject(reply);
                    reply = nullptr;
                }
            }

            redisReply* operator-> () {
                return reply;
            }

            redisReply* operator* () {
                return reply;
            }

            bool isNull() const {return reply == NULL; }

            bool isSuccess() { //检查是否是OK
                bool result = false;
                if(!isNull() && reply->type != REDIS_REPLY_ERROR) {
                    if(reply->type == REDIS_REPLY_STATUS) {
                        result = MyCommon::upper( string(reply->str)) == "OK";
                    }
                }
                return result;
            }

            bool hasError() { //是否有错误
                return isNull() || reply->type == REDIS_REPLY_ERROR;
            }

            string getErrStr() const {
                return isNull() || reply->type != REDIS_REPLY_ERROR ? "" : reply->str;
            }

        private:
            redisReply* reply; //redis处理结果
        };

        typedef std::shared_ptr<RedisReplyHelper> RedisReplyHelperPtr;

        /*
         * @brief Redis操作类，非线程安全
         */
        class DCRedis {
        public:

            /*
             * 构造函数
             */
            DCRedis(const DCRedisConf& conf);

            /*
             * 析构函数
             */
            ~DCRedis();

            /*
             * 操作原始命令
             */
            template<typename... Args>
            RedisReplyHelperPtr exec(const string& command, Args... args);

            /*
             * 查询
             */
            string get(const string& key);

            /*
             * 获取一个string的range
             */
            string get_range(const string& key, int32_t begin, int32_t end);

            /*
             * 设置值, 并且设置超时时间, tv = NULL 不超时, 自动创建
             */
            bool set(const string& key, const string& value, bool create, timeval* tv);

            /*
             * 按范围设置
             */
            bool set_range(const string& key, uint32_t offset, const string& value);

            /*
             * 设置新值并且返回旧值
             */
            string getset(const string& key, const string& value);

            /*
             * 追加数据
             */
            bool append(const string& key, const string& value);

            /*
             * 检查key是否存在
             */
            bool exists(const string& key);

            /*
             * 删除
             */
            bool del(const string& key);
            int64_t del(const vector<string>& key);

            /*
             * hash delete
             */
            int64_t hash_del(const string& key, const vector<string>& fields);

            /*
             * 设置超时信息
             */
            bool expire(const string& key, uint64_t expire);
            bool expire_at(const string& key, uint32_t timestamp);

            ////////////////////////////////////////////////////////////////////////////////////////
            //hashmap和map相关接口

            /*
             * hash set
             */
            bool hset(const string& key, const string& field, const string& value, struct timeval* tv);

            /*
             * mset("1", "f1", "1", "f2", "2");
             * mset("1", pair<"a", "2">)
             */
            bool mset(const map<string, string> &keys);
            bool hmset(const string& key, const map<string, string>& fields);

            /*
             * field是否存在
             */
            bool hexists(const string& key, const string& field);

            /*
             * hash get
             */
            string hget(const string& key, const string& field);

            /*
             *  查询多个key
             */
            map<string, string> mget(const vector<string>& keys);

            /*
             * 查询多个hash的key
             */
            map<string, string> hmget(const string& key, const vector<string>& fields );

            /*
             * 查询所有的key
             */
            map<string, string> hgetall(const string& key);

            /*
             * 获取所有的key、value和元素个数
             */
            vector<string> hkeys(const string& key); //返回所有的field
            vector<string> hvalues(const string& key); //返回所有的values
            int64_t hlen(const string& key); //返回hashmap的元素个数


            ////////////////////////////////////////////////////////////////////////////////////////
            //bitmap相关接口

            /*
             * 自增和自减, 失败返回-1
             */
            int64_t incr(const string& key, int32_t value = 1);
            int64_t hincr(const string& key, const string& field, int32_t value = 1);

            int64_t decr(const string& key, int32_t value = 1);
            int64_t hdecr(const string& key, const string& field, int32_t value = 1);

            /*
             *TODO: list相关接口
             */

            int64_t lpush(const string& key, const string& value);
            int64_t rpush(const string& key, const string& value);
            int64_t linsert(const string& key, const string& before, const string& value);
            bool lset(const string& key, const string& value, int32_t index);

            string lpop(const string& key);
            string rpop(const string& key);
            vector<string> lrange(const string& key, int32_t begin, int32_t end);

            int64_t lrem(const string& key, int32_t count, const string& begin);
            bool ltrim(const string& key, int32_t begin, int32_t end);

            string lindex(const string& key, int32_t index);
            int64_t llen(const string& key);

            /*
             * TODO: ZMap相关接口
             */

            /**
             *  获取最近执行的command
             *
             *  @return 最近执行的命令
             */
            string get_last_command() const {return last_command_;};

            /**
             *  获取最近执行的错误
             *
             *  @return 执行的错误信息
             */
            string get_last_error() const {return last_error_;}

            uint32_t last_reconnect_count() const {return last_reconnect_count_;}

        protected:

            /*
             * 重连
             */
            int32_t connect(const string& host, uint16_t port, uint32_t database) ;

            /*
             * 断开连接
             */
            void disconnect();
        private:
            redisContext* redisContext; //redis连接

            DCRedisConf config; //redis的配置

            string lastCommand; //上次执行的命令

            string lastError; //上次执行的错误

            uint32_t lastReconnectTime; //上次执行过程中，重连的次数

        };

        template<typename... Args>
        RedisReplyHelperPtr DCRedis::exec(const std::string& command, Args... args) {
            redisReply* reply = nullptr;
            RedisReplyHelperPtr reply_helper = nullptr;

            if (redisContext == nullptr) {
                LOG(ERROR) << "redis not connected, host: " << config.host << ", port: " << config.port << std::endl;
                return nullptr;
            }

            //构造参数
            char** argv = new char* [sizeof...(args) + 1];


            reply = (redisReply*)redisCommandArgv(redisContext, arg->argc(), arg->argv(), nullptr);

            if (reply == nullptr
                || redis_context_->err == REDIS_ERR_IO //发生了连接错误
                || redis_context_->err == REDIS_ERR_EOF
                || redis_context_->err == REDIS_ERR_PROTOCOL
                || redis_context_->err == REDIS_ERR_OTHER) {
                //重连
                disconnect();
                connect(conf_.host_, conf_.port_, conf_.database_);

            reply_helper = std::make_shared<RedisReplyHelper>(reply);
            last_error_ = reply_helper->error_str();
            return reply_helper;
        }
    }
}


#endif /* defined(__DBPoolTest__dcredis__) */
