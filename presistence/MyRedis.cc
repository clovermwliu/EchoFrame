//
//  dcredis.cpp
//  DBPoolTest
//
//  Created by mingweiliu on 16/1/22.
//  Copyright (c) 2016年 mingweiliu. All rights reserved.
//

#include "util/MyCommon.h"
#include "presistence/MyRedis.h"
namespace MF {
    namespace Presistence {
        DCRedis::DCRedis(const DCRedisConf& conf) {
            this->config = conf;
            redisContext = nullptr;
            connect(config.host, config.port, config.db);
        }

        DCRedis::~DCRedis() {
            disconnect();
        }


        string DCRedis::get(const string &key)
        {
            string value;

            DCRedisArg arg;
            arg << "GET" << key;
            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->isnull()) {
                if ((*reply)->type == REDIS_REPLY_INTEGER) {
                    value = DCCommon::tostr((*reply)->integer);
                } else if ((*reply)->type == REDIS_REPLY_STRING) {
                    value.assign((*reply)->str, (*reply)->len);
                } else {
                    // 其他了类型不处理
                }
            }

            return value;
        }

        string DCRedis::get_range(const string &key, int32_t begin, int32_t end)
        {
            string value;

            DCRedisArg arg;
            arg << "GETRANGE" << key << DCCommon::tostr(begin) << DCCommon::tostr(end);
            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->isnull()) {
                if ((*reply)->type == REDIS_REPLY_STRING) {
                    value.assign((*reply)->str, (*reply)->len);
                } else {
                    // 其他了类型不处理
                }
            }

            return value;
        }

        bool DCRedis::set(const string &key, const string &value, bool create, timeval* tv)
        {
            DCRedisArg arg;
            arg << "SET" << key << value;
            if (!create) {
                arg << "XX";
            }
            if (tv != nullptr) {
                arg << "PX" << DCCommon::tostr(tv->tv_sec * 1000 + tv->tv_usec);
            }

            RedisReplyHelperPtr reply = exec(&arg);

            return reply->issucc();
        }

        bool DCRedis::set_range(const string &key, uint32_t offset, const string &value)
        {
            DCRedisArg arg;
            arg << "SETRANGE" << key << DCCommon::tostr(offset) << value;
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer > 0;
        }

        string DCRedis::getset(const string &key, const string &value)
        {
            string old_value ;

            DCRedisArg arg;
            arg << "GETSET" << key << value;
            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->isnull()) {
                if ((*reply)->type == REDIS_REPLY_INTEGER) {
                    old_value = DCCommon::tostr((*reply)->integer) ;
                } else if ((*reply)->type == REDIS_REPLY_STRING) {
                    old_value.assign((*reply)->str, (*reply)->len);
                }
            }
            return old_value;
        }

        bool DCRedis::append(const string &key, const string &value)
        {
            DCRedisArg arg;
            arg << "APPEND" << key << value;
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer > 0;
        }

        bool DCRedis::exists(const string &key)
        {
            DCRedisArg arg;
            arg << "EXISTS" << key;
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer == 1;
        }

        bool DCRedis::del(const string &key)
        {
            DCRedisArg arg;
            arg << "DEL" << key;
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer == 1;
        }

        int64_t DCRedis::del(const vector<string> &key)
        {
            DCRedisArg arg;
            arg << "DEL";
            for (auto i = 0; i < key.size(); ++i) {
                arg << key[i];
            }

            RedisReplyHelperPtr reply = exec(&arg);

            int64_t removed = 0;
            if (!reply->haserror()) {
                removed = (*reply)->integer;
            }

            return removed;
        }

        int64_t DCRedis::hash_del(const string &key, const vector<string>& fields)
        {
            int64_t ret = 0;

            DCRedisArg arg;
            arg << "HDEL" << key;
            for (auto i = 0; i < fields.size(); ++i) {
                arg << fields[i];
            }

            RedisReplyHelperPtr reply = exec(&arg);

            if(!reply->haserror()) {
                ret = (*reply)->integer; //返回成功删除的个数
            }

            return ret;
        }

        bool DCRedis::expire(const string &key, uint64_t expire)
        {
            DCRedisArg arg;
            arg << "EXPIRE" << key << DCCommon::tostr(expire);

            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer == 1;
        }

        bool DCRedis::expire_at(const string &key, uint32_t timestamp)
        {
            DCRedisArg arg;
            arg << "EXPIREAT" << key << DCCommon::tostr(timestamp);
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer == 1;
        }

        bool DCRedis::hset(const string &key, const string &field, const string &value, struct timeval* tv)
        {
            bool result = false;

            DCRedisArg arg;
            arg << "HSET" << key << field << value;
            RedisReplyHelperPtr reply = exec(&arg);

            result = !reply->haserror();

            if (result) {
                if (tv != nullptr && !expire(key, tv->tv_sec + tv->tv_usec / 1000)) { //设置超时失败了
                    del(key);
                    result = false;
                }
            }
            return result;
        }

        bool DCRedis::mset(const map<string, string> &keys)
        {
            DCRedisArg arg;
            arg << "MSET";
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                arg << it->first << it->second;
            }

            RedisReplyHelperPtr reply = exec(&arg);

            return reply->issucc();
        }

//    int64_t DCRedis::mset(const string &key, const char *field, ...)
//    {
//        int64_t ret = 0;
//
//        string cmd = "MSET " + key;
//
//        va_list arg_list;
//        va_start(arg_list, field);
//
//        while (true) {
//            const char* field = va_arg(arg_list, const char* );
//            if (field == NULL) {
//                break;
//            }
//            const char* value = va_arg(arg_list, const char*);
//            if (value == NULL) {
//                break;
//            }
//
//            cmd += " " + string(field) + " " + string(value) ;
//        }
//
//        va_end(arg_list);
//
//        redisReply* reply = exec(cmd);
//
//        if (reply != nullptr && reply->type == REDIS_REPLY_INTEGER) {
//            ret = reply->integer;
//        }
//        return  ret;
//    }

//    int64_t DCRedis::hmset(const string &key, const char* field, ...)
//    {
//        int64_t ret = 0;
//
//        string cmd = "HMSET " + key;
//
//        va_list arg_list;
//        va_start(arg_list, field);
//
//        while (true) {
//            const char* field = va_arg(arg_list, const char* );
//            if (field == NULL) {
//                break;
//            }
//            const char* value = va_arg(arg_list, const char*);
//            if (value == NULL) {
//                break;
//            }
//
//            cmd += " " + string(field) + " " + string(value) ;
//        }
//
//        va_end(arg_list);
//
//        redisReply* reply = exec(cmd);
//
//        if (reply != nullptr && reply->type == REDIS_REPLY_INTEGER) {
//            ret = reply->integer;
//        }
//        return  ret;
//    }

        bool DCRedis::hmset(const string &key, const map<string, string> &fields)
        {
            DCRedisArg arg;
            arg << "HMSET" << key;
            for (auto it = fields.begin(); it != fields.end(); ++it) {
                arg << it->first << it->second;
            }

            RedisReplyHelperPtr reply = exec(&arg);

            return reply->issucc();
        }

        bool DCRedis::hexists(const string &key, const string &field)
        {
            DCRedisArg arg;
            arg << "HEXISTS" << key << field;
            RedisReplyHelperPtr reply = exec(&arg);

            return !reply->haserror() && (*reply)->integer == 1;
        }

        string DCRedis::hget(const string &key, const string &field)
        {
            string value;

            DCRedisArg arg;
            arg << "HGET" << key << field;
            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->haserror()) {
                if ((*reply)->type == REDIS_REPLY_INTEGER) {
                    value = DCCommon::tostr((*reply)->integer);
                } else if ((*reply)->type == REDIS_REPLY_STRING) {
                    value.assign((*reply)->str, (*reply)->len);
                }
            }

            return value;
        }

//    map<string, string> DCRedis::mget(const char* key, ...)
//    {
//        map<string, string> values;
//        vector<string> keys;
//
//        va_list arg_list;
//        va_start(arg_list, key);
//
//        string cmd = "MGET " ;
//        while (true) {
//            const char* field = va_arg(arg_list, const char*);
//            if (field == nullptr) {
//                break;
//            }
//            cmd += " " + string(field);
//            keys.push_back(field);
//        }
//
//        redisReply* reply = exec(cmd);
//
//        if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY) {
//            for (auto i = 0; i < reply->elements; ++i) {
//                redisReply* child = reply->element[i];
//                if (child->type == REDIS_REPLY_INTEGER) {
//                    values.insert(make_pair(keys[i], DCCommon::tostr(reply->integer)));
//                } else if (child->type == REDIS_REPLY_STRING) {
//                    values.insert(make_pair(keys[i], string(reply->str, reply->len)));
//                }
//            }
//        }
//
//        return values;
//    }

        map<string, string> DCRedis::mget(const vector<string>& keys)
        {
            map<string, string> values;

            DCRedisArg arg;
            arg << "MGET";

            for (auto i = 0; i < keys.size(); ++i) {
                arg << keys[i];
            }

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->isnull() && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; ++i) {
                    redisReply* child = (*reply)->element[i];
                    if (child->type == REDIS_REPLY_INTEGER) {
                        values.insert(make_pair(keys[i], DCCommon::tostr(child->integer)));
                    } else if (child->type == REDIS_REPLY_STRING) {
                        values.insert(make_pair(keys[i], string(child->str, child->len)));
                    }
                }
            }

            return values;
        }

        map<string, string> DCRedis::hmget(const string& key, const vector<string>& fields )
        {
            map<string, string> values;

            DCRedisArg arg;
            arg << "HMGET" << key;

            for (auto i = 0; i < fields.size(); ++i) {
                arg << fields[i];
            }

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->isnull() && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; ++i) {
                    redisReply* child = (*reply)->element[i];
                    if (child->type == REDIS_REPLY_INTEGER) {
                        values.insert(make_pair(fields[i], DCCommon::tostr(child->integer)));
                    } else if (child->type == REDIS_REPLY_STRING) {
                        values.insert(make_pair(fields[i], string(child->str, child->len)));
                    }
                }
            }

            return values;
        }

        map<string, string> DCRedis::hgetall(const string &key)
        {
            map<string, string> values;

            DCRedisArg arg;
            arg << "HGETALL" <<key;
            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->isnull() && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; i += 2) {
                    redisReply* child_name = (*reply)->element[i];
                    redisReply* child_value = (*reply)->element[i + 1];
                    if (child_value->type == REDIS_REPLY_INTEGER) {
                        values.insert(make_pair(string(child_name->str, child_name->len), DCCommon::tostr(child_value->integer)));
                    } else if (child_value->type == REDIS_REPLY_STRING) {
                        values.insert(make_pair(string(child_name->str, child_name->len), string(child_value->str, child_value->len)));
                    }
                }
            }

            return values;
        }

        vector<string> DCRedis::hkeys(const string &key)
        {
            vector<string> fields;

            DCRedisArg arg;
            arg << "HKEYS" << key;
            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->isnull() && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; ++i) {
                    redisReply* child = (*reply)->element[i];
                    fields.push_back(string(child->str, child->len));
                }
            }

            return fields;
        }

        vector<string> DCRedis::hvalues(const string &key)
        {
            vector<string> fields;

            DCRedisArg arg;
            arg << "HVALS" << key;
            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->isnull() && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; ++i) {
                    redisReply* child = (*reply)->element[i];
                    fields.push_back(string(child->str, child->len));
                }
            }

            return fields;
        }

        int64_t DCRedis::hlen(const string &key)
        {
            int64_t len = 0;

            DCRedisArg arg;
            arg << "HLEN" << key;
            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                len = (*reply)->integer;
            }
            return len;
        }

        int64_t DCRedis::incr(const string &key, int32_t value)
        {
            int64_t new_value = -1;

            DCRedisArg arg;
            arg << "INCRBY" << key << DCCommon::tostr(value);

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->haserror()) {
                new_value = (*reply)->integer;
            }

            return new_value;
        }

        int64_t DCRedis::hincr(const string &key, const string &field, int32_t value)
        {
            int64_t new_value = -1;

            DCRedisArg arg;
            arg << "HINCRBY" << key << field << DCCommon::tostr(value);

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->haserror()) {
                new_value = (*reply)->integer;
            }

            return new_value;
        }

        int64_t DCRedis::decr(const string &key, int32_t value)
        {
            int64_t new_value = -1;

            DCRedisArg arg;
            arg << "DECRBY" << key << DCCommon::tostr(value);

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->haserror()) {
                new_value = (*reply)->integer;
            }

            return new_value;
        }

        int64_t DCRedis::hdecr(const string &key, const string &field, int32_t value)
        {
            int64_t new_value = -1;
            DCRedisArg arg;
            arg << "HINCRBY" << key << field << DCCommon::tostr(value * -1);

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->haserror()) {
                new_value = (*reply)->integer;
            }

            return new_value;
        }

        int64_t DCRedis::lpush(const string& key, const string& value) {
            int64_t ret = -1; //-1表示错误

            DCRedisArg arg;
            arg<< "LPUSH" << key << value;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                ret = (*reply)->integer;
            }

            return ret;
        }

        int64_t DCRedis::rpush(const string& key, const string& value) {
            int64_t ret = -1; //-1表示错误

            DCRedisArg arg;
            arg<< "RPUSH" << key << value;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                ret = (*reply)->integer;
            }

            return ret;
        }

        int64_t DCRedis::linsert(const string& key, const string& before, const string& value) {

            int64_t ret = -1; //-1表示错误

            DCRedisArg arg;
            arg<< "LINSERT" << key << "BEFORE" << before << value;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                ret = (*reply)->integer;
            }

            return ret;
        }

        bool DCRedis::lset(const string& key, const string& value, int32_t index) {

            DCRedisArg arg;
            arg<< "LSET" << key << DCCommon::tostr(index) << value;

            RedisReplyHelperPtr reply = exec(&arg);

            return !(reply->haserror());
        }

        string DCRedis::lpop(const string& key) {
            string value;

            DCRedisArg arg;
            arg<< "LPOP" << key;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()
                && (*reply)->type == REDIS_REPLY_STRING) {
                value = (*reply)->str;
            }

            return value;
        }

        string DCRedis::rpop(const string& key) {

            string value;

            DCRedisArg arg;
            arg<< "RPOP" << key;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()
                && (*reply)->type == REDIS_REPLY_STRING) {
                value = (*reply)->str;
            }

            return value;
        }

        vector<string> DCRedis::lrange(const string& key, int32_t begin, int32_t end){

            vector<string> values;

            DCRedisArg arg;
            arg << "LRANGE" << key << DCCommon::tostr(begin) << DCCommon::tostr(end);

            RedisReplyHelperPtr reply = exec(&arg);

            if (!reply->isnull()
                && (*reply)->type == REDIS_REPLY_ARRAY) {
                for (auto i = 0; i < (*reply)->elements; ++i) {
                    redisReply* child = (*reply)->element[i];
                    if (child->type == REDIS_REPLY_STRING) {
                        values.push_back(string(child->str, child->len));
                    }
                }
            }

            return values;
        }

        int64_t DCRedis::lrem(const string& key, int32_t count, const string& begin) {

            int64_t ret = -1; //-1表示错误

            DCRedisArg arg;
            arg<< "LREM" << key << DCCommon::tostr(count) << begin;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                ret = (*reply)->integer;
            }

            return ret;
        }

        bool DCRedis::ltrim(const string& key, int32_t begin, int32_t end) {

            DCRedisArg arg;
            arg<< "LTRIM" << key << DCCommon::tostr(begin) << DCCommon::tostr(end);

            RedisReplyHelperPtr reply = exec(&arg);

            return !(reply->haserror());
        }

        string DCRedis::lindex(const string& key, int32_t index) {

            string value;

            DCRedisArg arg;
            arg<< "LINDEX" << key << DCCommon::tostr(index);

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()
                && (*reply)->type == REDIS_REPLY_STRING) {
                value = (*reply)->str;
            }

            return value;
        }

        int64_t DCRedis::llen(const string& key) {

            int64_t ret = -1; //-1表示错误

            DCRedisArg arg;
            arg<< "LLEN" << key;

            RedisReplyHelperPtr reply = exec(&arg);
            if (!reply->haserror()) {
                ret = (*reply)->integer;
            }

            return ret;
        }

        int32_t DCRedis::connect(const string& host, uint16_t port, uint32_t database)
        {
            struct timeval tv;
            tv.tv_sec = 3; //等待3秒
            tv.tv_usec = 0;
            redis_context_ = redisConnectWithTimeout(host.c_str(), port, tv);
            last_reconnect_count_++; //增加重连次数
            if (redisContext == nullptr || redisContext->err != 0) {
                auto err = redisContext == nullptr ? "Unknown error" : redisContext->errstr;
                LOG(ERROR) << "redis connect to " << config.host
                           << ":" << config.port << " fail"
                           << ", error: " << err << std::endl;
                return -1;
            } else {
                LOG(INFO) << "redis connect to " << config.host << ":" << config.port << " success" << std::endl;
                //设置保持连接
                redisEnableKeepAlive(redisContext);
//                //成功了, 设置database
//                redisReply* reply = (redisReply*)redisCommand(redisContext, "select %u", database);
//                //释放reply
//                freeReplyObject(reply);
            }
        }

        void DCRedis::disconnect() {
            if (redisContext != nullptr) {
                redisFree(redisContext);
                redisContext = nullptr;
            }
        }
    }
}

